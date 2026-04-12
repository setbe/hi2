#define IO_IMPLEMENTATION
#include "../../hi/io.hpp"

namespace demo {

    using u32 = io::u32;
    using u64 = io::u64;

    static inline u64 mix64(u64 x) noexcept {
        // simple spinner: SplitMix64-like avalanche (CPU heavy)
        x += 0x9e3779b97f4a7c15ull;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
        x ^= (x >> 31);
        return x;
    }

    struct WorkItem {
        u64 seed;
        u32 iters;
        io::atomic<u64>* sink;          // compiler does not discard the calculation
        io::atomic<u32>* remaining;     // completed tasks counter
    };

    static void cpu_task(void* p) noexcept {
        auto* w = (WorkItem*)p;

        u64 x = w->seed;
        // CPU-bound loop
        for (u32 i = 0; i < w->iters; ++i) {
            x = mix64(x);
        }

        // side-effect
        w->sink->fetch_add(x, io::memory_order_relaxed);
        w->remaining->fetch_sub(1u, io::memory_order_release);
    }

    static inline void spin_wait_done(io::atomic<u32>& remaining) noexcept {
        // Waiting for all tasks to complete.
        // For VTune, it is useful to have a predictable wait.
        while (remaining.load(io::memory_order_acquire) != 0u) {
            // very light backoff
            for (volatile int k = 0; k < 500; ++k) {}
        }
    }

} // namespace demo

int main() {
    using namespace demo;

    // --- ideal threading layout ---
    // 1 main thread submits work
    // N worker threads execute work, where N == number of online logical CPUs
    const unsigned workers = io::Thread::max_workers();

    // --- experimental parameters ---
    const u32 tasks = 200000;                // tasks count
    const u32 iters_per_task = 10000;        // "difficulty" of the task (CPU)
    const unsigned queue_cap = 16384;        // MUST be power-of-two; keep it large to reduce submit backpressure

    // --- data ---
    io::atomic<u64> sink{ 0 };
    io::atomic<u32> remaining{ tasks };

    auto* items = (WorkItem*)::operator new(sizeof(WorkItem) * (size_t)tasks, std::nothrow);
    if (!items) return 2;

    for (u32 i = 0; i < tasks; ++i) {
        items[i] = WorkItem{
            0x123456789abcdef0ull ^ (u64)i,
            iters_per_task,
            &sink,
            &remaining
        };
    }

    io::ThreadPool pool;
    if (!pool.init(workers, queue_cap)) return 3;

    // --- submit all tasks from main thread ---
    for (u32 i = 0; i < tasks; ++i) {
        // MPMC submit may return false if the queue is temporarily full -> backoff
        while (!pool.submit(&cpu_task, &items[i])) {
            // light spin so as not to prevent VTune from seeing contention
            for (volatile int k = 0; k < 200; ++k) {}
        }
    }

    // --- wait all tasks done ---
    spin_wait_done(remaining);

    // graceful shutdown (drain already done)
    pool.shutdown(true);

    ::operator delete(items);
    return (sink.load(io::memory_order_relaxed) != 0ull) ? 0 : 0;
}