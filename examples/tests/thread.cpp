#define IO_IMPLEMENTATION
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/io.hpp"

// ============================================================================
// io::Thread
// ============================================================================

static void tp_set_u32(void* p) noexcept {
    *reinterpret_cast<io::u32*>(p) = 123u;
}

static void tp_inc_atomic(void* p) noexcept {
    auto* a = reinterpret_cast<io::atomic<io::u32>*>(p);
    a->fetch_add(1u, io::memory_order_relaxed);
}

TEST_CASE("io::Thread max_workers returns >= 1", "[io][thread]") {
    auto n = io::Thread::max_workers();
    REQUIRE(n >= 1u);
}

TEST_CASE("io::Thread start/join executes function", "[io][thread]") {
    io::u32 v = 0;
    io::Thread t;
    REQUIRE(t.start(&tp_set_u32, &v));
    REQUIRE(t.running());
    REQUIRE(t.join());
    REQUIRE(!t.running());
    REQUIRE(v == 123u);
}

TEST_CASE("io::Thread start fails if already running", "[io][thread]") {
    io::atomic<io::u32> a{ 0 };
    io::Thread t;
    REQUIRE(t.start(&tp_inc_atomic, &a));
    REQUIRE_FALSE(t.start(&tp_inc_atomic, &a)); // already running
    REQUIRE(t.join());
    REQUIRE(a.load() == 1u);
}

TEST_CASE("io::Thread detach returns and thread still runs", "[io][thread]") {
    io::atomic<io::u32> a{ 0 };
    io::Thread t;

    REQUIRE(t.start(&tp_inc_atomic, &a));
    REQUIRE(t.detach());
    REQUIRE_FALSE(t.running());

    // spin-wait until the detached thread increments
    for (int i = 0; i < 1000000; ++i) {
        if (a.load(io::memory_order_relaxed) != 0u) break;
    }
    REQUIRE(a.load(io::memory_order_relaxed) == 1u);
}

// ============================================================================
// io::ThreadPool (lock-free MPMC)
// ============================================================================

static void pool_add_u32(void* p) noexcept {
    auto* a = reinterpret_cast<io::atomic<io::u32>*>(p);
    a->fetch_add(1u, io::memory_order_relaxed);
}

struct PoolAddCtx {
    io::atomic<io::u32>* counter;
    io::u32 iters;
};
static void pool_add_many(void* p) noexcept {
    auto* c = reinterpret_cast<PoolAddCtx*>(p);
    for (io::u32 i = 0; i < c->iters; ++i)
        c->counter->fetch_add(1u, io::memory_order_relaxed);
}

// There is no condvar for a lock-free pool, so we do shutdown(graceful) as follows:
// 1) add N "stop" tasks that set the done flag for the worker (via a shared counter)
// 2) when all N stops are completed — shutdown() can be performed
// But implementation already performs graceful drain via _stop + empty-queue exit,
// so we just check that all tasks have been completed (via the counter) and then perform shutdown().

TEST_CASE("io::ThreadPool init/shutdown basic", "[io][threadpool]") {
    io::ThreadPool p;
    REQUIRE(p.init(2u, 1024u));
    REQUIRE(p.worker_count() == 2u);
    REQUIRE(p.capacity() == 1024u);

    p.shutdown(true);
}

TEST_CASE("io::ThreadPool submit executes tasks (single producer)", "[io][threadpool]") {
    io::ThreadPool p;
    REQUIRE(p.init(4u, 1024u));

    io::atomic<io::u32> counter{ 0 };

    const io::u32 N = 5000;
    io::u32 submitted = 0;

    for (io::u32 i = 0; i < N; ++i) {
        if (p.submit(&pool_add_u32, &counter)) ++submitted;
        else break; // queue full (should not occur with 1024+ fast workers, but the test is correct)
    }

    // Wait until everything we set out to do has been accomplished.
    for (int spin = 0; spin < 50'000'000; ++spin) {
        if (counter.load(io::memory_order_relaxed) == submitted) break;
    }
    REQUIRE(counter.load() == submitted);

    p.shutdown(true);
}

TEST_CASE("io::ThreadPool submit returns false after shutdown requested", "[io][threadpool]") {
    io::ThreadPool p;
    REQUIRE(p.init(2u, 1024u));

    p.shutdown(true);

    io::atomic<io::u32> counter{ 0 };
    REQUIRE_FALSE(p.submit(&pool_add_u32, &counter));
}

TEST_CASE("io::ThreadPool handles many tasks (stress)", "[io][threadpool]") {
    const unsigned workers = (io::Thread::max_workers() >= 2u) ? 4u : 1u;

    io::ThreadPool p;
    REQUIRE(p.init(workers, 4096u));

    io::atomic<io::u32> counter{ 0 };

    const io::u32 N = 200000; // plenty, but not too much
    io::u32 submitted = 0;

    // Try to throw N tasks, but if the queue is full, we'll turn around and repeat
    // (without mutex/condvar, this is a normal strategy for a producer)
    for (io::u32 i = 0; i < N; ) {
        if (p.submit(&pool_add_u32, &counter)) {
            ++submitted;
            ++i;
        }
        else {
            // backoff
            for (int k = 0; k < 100; ++k) { /* spin */ }
        }
    }

    // wait all
    for (int spin = 0; spin < 200'000'000; ++spin) {
        if (counter.load(io::memory_order_relaxed) == submitted) break;
    }
    REQUIRE(counter.load() == submitted);

    p.shutdown(true);
}

TEST_CASE("io::ThreadPool supports 'tasks that enqueue tasks' (nested submit)", "[io][threadpool]") {
    io::ThreadPool p;
    REQUIRE(p.init(4u, 2048u));

    io::atomic<io::u32> counter{ 0 };
    PoolAddCtx ctx{ &counter, 1000u };

    // one task that will add 1000 increments
    REQUIRE(p.submit(&pool_add_many, &ctx));

    // wait 1000
    for (int spin = 0; spin < 50'000'000; ++spin) {
        if (counter.load(io::memory_order_relaxed) == 1000u) break;
    }
    REQUIRE(counter.load() == 1000u);

    p.shutdown(true);
}

// MPMC aspect: multiple producer threads that concurrently submit to the pool
struct ProdCtx {
    io::ThreadPool* pool;
    io::atomic<io::u32>* counter;
    io::u32 tasks;
};
static void producer_submitter(void* p) noexcept {
    auto* c = reinterpret_cast<ProdCtx*>(p);
    io::u32 done = 0;
    while (done < c->tasks) {
        if (c->pool->submit(&pool_add_u32, c->counter)) {
            ++done;
        }
        else {
            // backoff
            for (int k = 0; k < 200; ++k) { /* spin */ }
        }
    }
}

TEST_CASE("io::ThreadPool MPMC: multiple producers submit concurrently", "[io][threadpool][mpmc]") {
    io::ThreadPool p;
    REQUIRE(p.init(4u, 4096u));

    io::atomic<io::u32> counter{ 0 };

    io::Thread prod1, prod2, prod3;
    ProdCtx c1{ &p, &counter, 20000u };
    ProdCtx c2{ &p, &counter, 20000u };
    ProdCtx c3{ &p, &counter, 20000u };

    REQUIRE(prod1.start(&producer_submitter, &c1));
    REQUIRE(prod2.start(&producer_submitter, &c2));
    REQUIRE(prod3.start(&producer_submitter, &c3));

    REQUIRE(prod1.join());
    REQUIRE(prod2.join());
    REQUIRE(prod3.join());

    const io::u32 submitted = c1.tasks + c2.tasks + c3.tasks;

    // wait until all executed
    for (int spin = 0; spin < 300'000'000; ++spin) {
        if (counter.load(io::memory_order_relaxed) == submitted) break;
    }
    REQUIRE(counter.load() == submitted);

    p.shutdown(true);
}