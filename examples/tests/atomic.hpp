#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/io.hpp"

TEST_CASE("io::atomic default ctor loads default-initialized value", "[io][atomic]") {
    io::atomic<int> a{};
    a.store(0);
    REQUIRE(a.load() == 0);
}

TEST_CASE("io::atomic store/load roundtrip", "[io][atomic]") {
    io::atomic<int> a{ 123 };
    REQUIRE(a.load() == 123);

    a.store(456);
    REQUIRE(a.load() == 456);

    a.store(-7, io::memory_order_relaxed);
    REQUIRE(a.load(io::memory_order_relaxed) == -7);
}

TEST_CASE("io::atomic implicit conversion and assignment operator", "[io][atomic]") {
    io::atomic<int> a{ 5 };

    int x = a;               // operator T()
    REQUIRE(x == 5);

    a = 9;                   // operator=(T)
    REQUIRE(a.load() == 9);
}

TEST_CASE("io::atomic exchange returns old value and stores new", "[io][atomic]") {
    io::atomic<int> a{ 10 };

    int old = a.exchange(77);
    REQUIRE(old == 10);
    REQUIRE(a.load() == 77);

    old = a.exchange(-3, io::memory_order_relaxed);
    REQUIRE(old == 77);
    REQUIRE(a.load() == -3);
}

TEST_CASE("io::atomic compare_exchange_strong success", "[io][atomic]") {
    io::atomic<int> a{ 42 };

    int expected = 42;
    bool ok = a.compare_exchange_strong(expected, 99);
    REQUIRE(ok);
    REQUIRE(a.load() == 99);
    REQUIRE(expected == 42); // expected unchanged on success
}

TEST_CASE("io::atomic compare_exchange_strong failure updates expected", "[io][atomic]") {
    io::atomic<int> a{ 11 };

    int expected = 10;
    bool ok = a.compare_exchange_strong(expected, 22);
    REQUIRE(!ok);
    REQUIRE(a.load() == 11);

    // On failure, expected must become actual value
    REQUIRE(expected == 11);
}

TEST_CASE("io::atomic fetch_add returns old and adds", "[io][atomic]") {
    io::atomic<int> a{ 1 };

    int old = a.fetch_add(3);
    REQUIRE(old == 1);
    REQUIRE(a.load() == 4);

    old = a.fetch_add(-2, io::memory_order_relaxed);
    REQUIRE(old == 4);
    REQUIRE(a.load() == 2);
}

TEST_CASE("io::atomic fetch_sub returns old and subtracts", "[io][atomic]") {
    io::atomic<int> a{ 10 };

    int old = a.fetch_sub(4);
    REQUIRE(old == 10);
    REQUIRE(a.load() == 6);

    old = a.fetch_sub(-3, io::memory_order_relaxed);
    REQUIRE(old == 6);
    REQUIRE(a.load() == 9);
}

TEST_CASE("io::atomic fetch_and returns old and ANDs", "[io][atomic]") {
    io::atomic<unsigned> a{ 0b1101u };

    unsigned old = a.fetch_and(0b0110u);
    REQUIRE(old == 0b1101u);
    REQUIRE(a.load() == (0b1101u & 0b0110u));

    old = a.fetch_and(0b1111u, io::memory_order_relaxed);
    REQUIRE(old == (0b1101u & 0b0110u));
    REQUIRE(a.load() == ((0b1101u & 0b0110u) & 0b1111u));
}

TEST_CASE("io::atomic fetch_or returns old and ORs", "[io][atomic]") {
    io::atomic<unsigned> a{ 0b0101u };

    unsigned old = a.fetch_or(0b0011u);
    REQUIRE(old == 0b0101u);
    REQUIRE(a.load() == (0b0101u | 0b0011u));

    old = a.fetch_or(0u, io::memory_order_relaxed);
    REQUIRE(old == (0b0101u | 0b0011u));
    REQUIRE(a.load() == (0b0101u | 0b0011u));
}

TEST_CASE("io::atomic works with small integer types", "[io][atomic]") {
    io::atomic<io::u32> a{ 1u };
    REQUIRE(a.load() == 1u);

    auto old = a.fetch_add(2u);
    REQUIRE(old == 1u);
    REQUIRE(a.load() == 3u);

    io::u32 expected = 3u;
    REQUIRE(a.compare_exchange_strong(expected, 9u));
    REQUIRE(a.load() == 9u);
}
