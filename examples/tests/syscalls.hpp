#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/io.hpp"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#endif

// ---------------------- helpers ----------------------

static bool can_write_bytes(void* p, io::usize n) {
    volatile unsigned char* b = static_cast<volatile unsigned char*>(p);
    for (io::usize i = 0; i < n; ++i) b[i] = static_cast<unsigned char>(i);
    for (io::usize i = 0; i < n; ++i) if (b[i] != static_cast<unsigned char>(i)) return false;
    return true;
}

// ---------------------- tests ------------------------

TEST_CASE("io::alloc returns writable memory and io::free releases it", "[io][syscalls][alloc]") {
    // test a few sizes including 0-ish and larger blocks
    const io::usize sizes[] = { 1, 8, 64, 4096, 65536 };

    for (io::usize sz : sizes) {
        void* p = io::alloc(sz);
        REQUIRE(p != nullptr);

        REQUIRE(can_write_bytes(p, (sz < 256 ? sz : 256))); // don't loop huge

        io::free(p);
        // We cannot safely dereference after free; just require no crash.
    }
}

TEST_CASE("io::alloc returns distinct regions for distinct allocations (likely)", "[io][syscalls][alloc]") {
    void* a = io::alloc(4096);
    void* b = io::alloc(4096);

    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);

    // Not a strict guarantee, but with VirtualAlloc/mmap it should be distinct.
    REQUIRE(a != b);

    io::free(a);
    io::free(b);
}

TEST_CASE("io::free(nullptr) is safe", "[io][syscalls][free]") {
    io::free(nullptr);
    SUCCEED();
}

TEST_CASE("io::monotonic_ms is non-decreasing", "[io][syscalls][time]") {
    const io::u64 t1 = io::monotonic_ms();
    const io::u64 t2 = io::monotonic_ms();
    const io::u64 t3 = io::monotonic_ms();

    REQUIRE(t2 >= t1);
    REQUIRE(t3 >= t2);
}

TEST_CASE("io::sleep_ms sleeps at least roughly the requested time", "[io][syscalls][sleep]") {
    const unsigned ms = 30;

    const io::u64 t1 = io::monotonic_ms();
    io::sleep_ms(ms);
    const io::u64 t2 = io::monotonic_ms();

    const io::u64 dt_ms = (t2 - t1); // already milliseconds

    REQUIRE(dt_ms >= 10);           // lower bound (scheduler granularity)
#ifdef _WIN32
    REQUIRE(dt_ms < 250);           // be tolerant on Windows/VM/CI
#else
    REQUIRE(dt_ms < 200);
#endif
}

TEST_CASE("io::exit_process terminates the process with given code (run in child)", "[io][syscalls][exit]") {
#ifdef _WIN32
    WARN("Skipping on Windows (needs custom main/argv or helper exe).");
    SUCCEED();
#else
    pid_t pid = fork();
    REQUIRE(pid != -1);

    if (pid == 0) {
        io::exit_process(123);
        _exit(255); // should never reach
    }

    int status = 0;
    REQUIRE(waitpid(pid, &status, 0) == pid);
    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 123);
#endif
}

TEST_CASE("io::secure_zero zeros memory", "[io][syscalls][crypto]") {
    io::u8 buf[256];
    for (io::usize i = 0; i < sizeof(buf); ++i) buf[i] = (io::u8)(i + 1);

    io::secure_zero(buf, (io::usize)sizeof(buf));

    for (io::usize i = 0; i < sizeof(buf); ++i) REQUIRE(buf[i] == 0);
}

TEST_CASE("io::os_entropy fills buffer with entropy (basic sanity)", "[io][syscalls][crypto]") {
    io::u8 a[32]{};
    io::u8 b[32]{};

    REQUIRE(io::os_entropy(a, (io::usize)sizeof(a)));
    REQUIRE(io::os_entropy(b, (io::usize)sizeof(b)));

    // Extremely unlikely to be all zeros; good sanity check.
    bool any_nonzero_a = false;
    for (io::usize i = 0; i < sizeof(a); ++i) any_nonzero_a |= (a[i] != 0);
    REQUIRE(any_nonzero_a);

    // Two reads should (almost surely) differ.
    bool any_diff = false;
    for (io::usize i = 0; i < sizeof(a); ++i) any_diff |= (a[i] != b[i]);
    REQUIRE(any_diff);
}

TEST_CASE("io::os_entropy size=0 is ok", "[io][syscalls][crypto]") {
    REQUIRE(io::os_entropy(nullptr, 0));
}
