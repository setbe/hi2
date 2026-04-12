#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/io.hpp"

// ------------------------------------------------------------
//              Compile-time sanity checks
// ------------------------------------------------------------
static_assert(sizeof(io::i8) == 1, "i8 must be 1 byte");
static_assert(sizeof(io::u8) == 1, "u8 must be 1 byte");
static_assert(sizeof(io::i16) == 2, "i16 must be 2 bytes");
static_assert(sizeof(io::u16) == 2, "u16 must be 2 bytes");
static_assert(sizeof(io::i32) == 4, "i32 must be 4 bytes");
static_assert(sizeof(io::u32) == 4, "u32 must be 4 bytes");
static_assert(sizeof(io::i64) == 8, "i64 must be 8 bytes");
static_assert(sizeof(io::u64) == 8, "u64 must be 8 bytes");
static_assert(sizeof(io::u128) == 16, "u128 must be 16 bytes");

static_assert(sizeof(io::usize) == sizeof(sizeof(0)), "usize must match sizeof(...) result type size");
static_assert(sizeof(io::isize) == sizeof(static_cast<char*>(nullptr) - static_cast<char*>(nullptr)),
    "isize must match pointer-difference expression type size");

// ------------------------------------------------------------
//                          Tests
// ------------------------------------------------------------
TEST_CASE("io::len counts bytes until NUL and handles nullptr", "[types][io][len]") {
    REQUIRE(io::len(nullptr) == 0);

    REQUIRE(io::len("") == 0);
    REQUIRE(io::len("a") == 1);
    REQUIRE(io::len("hello") == 5);

    const char s[] = "abc\0zzz";
    REQUIRE(io::len(s) == 3);
}

TEST_CASE("io::is_same_v works", "[types][io][traits]") {
    STATIC_REQUIRE(io::is_same_v<int, int>);
    STATIC_REQUIRE(!io::is_same_v<int, long>);
    STATIC_REQUIRE(io::is_same_v<io::true_t, io::constant<bool, true>>);
    STATIC_REQUIRE(io::is_same_v<io::false_t, io::constant<bool, false>>);
}

TEST_CASE("io::enable_if_t SFINAE basic usage compiles", "[types][io][sfinae]") {
    struct X {};

    auto f_int_only = [](auto v) -> io::enable_if_t<io::is_same_v<decltype(v), int>, int> {
        return v + 1;
    };

    REQUIRE(f_int_only(1) == 2);

    // Important: Do NOT call f_int_only(X{}), otherwise it will be a hard error, and we need compile-time SFINAE.
    SUCCEED();
}

TEST_CASE("io::remove_reference_t removes references", "[types][io][traits]") {
    STATIC_REQUIRE(io::is_same_v<io::remove_reference_t<int>, int>);
    STATIC_REQUIRE(io::is_same_v<io::remove_reference_t<int&>, int>);
    STATIC_REQUIRE(io::is_same_v<io::remove_reference_t<int&&>, int>);
}

TEST_CASE("io::move casts to rvalue ref (type-level check)", "[types][io][move]") {
    int x = 7;
    // Сheck the type of the result: remove_reference_t<T>&&
    STATIC_REQUIRE(io::is_same_v<decltype(io::move(x)), int&&>);
    (void)x;
    SUCCEED();
}

TEST_CASE("io::forward preserves value category (type-level checks)", "[types][io][forward]") {
    // lvalue forward
    int a = 1;
    STATIC_REQUIRE(io::is_same_v<decltype(io::forward<int&>(a)), int&>);

    // rvalue forward
    STATIC_REQUIRE(io::is_same_v<decltype(io::forward<int>(io::move(a))), int&&>);
    SUCCEED();
}

TEST_CASE("io::view default ctor is empty", "[types][io][view]") {
    io::view<int> v{};
    REQUIRE(v.data() == nullptr);
    REQUIRE(v.size() == 0);
    REQUIRE(v.empty());
}

TEST_CASE("io::view from pointer+len", "[types][io][view]") {
    int arr[] = { 1,2,3,4 };
    io::view<int> v(arr, 4);

    REQUIRE(v.size() == 4);
    REQUIRE(!v.empty());
    REQUIRE(v.data() == arr);
    REQUIRE(v[0] == 1);
    REQUIRE(v.back() == 4);
    REQUIRE(v.front() == 1);
}

TEST_CASE("io::view from C-array", "[types][io][view]") {
    int arr[] = { 10,20,30 };
    io::view<int> v(arr);

    REQUIRE(v.size() == 3);
    REQUIRE(v[1] == 20);
}

TEST_CASE("io::view cstr ctor works only for const char", "[types][io][view]") {
    io::view<const char> s("hello");
    REQUIRE(s.size() == 5);
    REQUIRE(s[0] == 'h');
    REQUIRE(s.back() == 'o');
}

TEST_CASE("io::view slicing helpers", "[types][io][view]") {
    int arr[] = { 1,2,3,4,5 };
    io::view<int> v(arr);

    auto first2 = v.first(2);
    REQUIRE(first2.size() == 2);
    REQUIRE(first2[0] == 1);
    REQUIRE(first2[1] == 2);

    auto last2 = v.last(2);
    REQUIRE(last2.size() == 2);
    REQUIRE(last2[0] == 4);
    REQUIRE(last2[1] == 5);

    auto drop3 = v.drop(3);
    REQUIRE(drop3.size() == 2);
    REQUIRE(drop3[0] == 4);
    REQUIRE(drop3[1] == 5);

    auto slice = v.slice(1, 3);
    REQUIRE(slice.size() == 3);
    REQUIRE(slice[0] == 2);
    REQUIRE(slice[1] == 3);
    REQUIRE(slice[2] == 4);

    auto oob1 = v.slice(100, 1);
    REQUIRE(oob1.size() == 0);

    auto too_much = v.first(999);
    REQUIRE(too_much.size() == 5);
}

// ------------------------------------------------------------
//                      u128 arithmetics
// ------------------------------------------------------------
static inline io::u128 make_u128(io::u64 hi, io::u64 lo) noexcept { return { lo, hi }; }

TEST_CASE("io::add_u128: without carry", "[u128][add]") {
    io::u128 a = make_u128(0x1111111111111111ull, 0x2222222222222222ull);
    io::u128 b = make_u128(0x0101010101010101ull, 0x0303030303030303ull);
    io::u128 r = io::add_u128(a, b);
    REQUIRE(r.lo == 0x2525252525252525ull);
    REQUIRE(r.hi == 0x1212121212121212ull);
}

TEST_CASE("io::add_u128: carry with lo in hi", "[u128][add][carry]") {
    io::u128 a = make_u128(0x0123456789abcdefull, 0xffffffffffffffffull);
    io::u128 b = make_u128(0x0000000000000000ull, 0x0000000000000001ull);
    io::u128 r = io::add_u128(a, b);
    REQUIRE(r.lo == 0x0000000000000000ull);
    REQUIRE(r.hi == 0x0123456789abcdf0ull);
}

TEST_CASE("io::add_u128_u64: without carry", "[u128][add_u64]") {
    io::u128 a = make_u128(0xaaaaaaaaaaaaaaaauLL, 0x1111111111111111ull);
    io::u64  b = 0x2222222222222222ull;
    io::u128 r = io::add_u128_u64(a, b);
    REQUIRE(r.lo == 0x3333333333333333ull);
    REQUIRE(r.hi == 0xaaaaaaaaaaaaaaaauLL);
}

TEST_CASE("io::add_u128_u64: carry with lo in hi", "[u128][add_u64][carry]") {
    io::u128 a = make_u128(0x123456789abcdef0ull, 0xfffffffffffffff0ull);
    io::u64  b = 0x30ull;
    io::u128 r = io::add_u128_u64(a, b);
    REQUIRE(r.lo == 0x0000000000000020ull);
    REQUIRE(r.hi == 0x123456789abcdef1ull);
}

TEST_CASE("io::mul_u64: trivial cases", "[u128][mul]") {
    {
        io::u128 r = io::mul_u64(0, 0);
        REQUIRE(r.lo == 0);
        REQUIRE(r.hi == 0);
    }
    {
        io::u128 r = io::mul_u64(0, 123);
        REQUIRE(r.lo == 0);
        REQUIRE(r.hi == 0);
    }
    {
        io::u128 r = io::mul_u64(1, 0xdeadbeefcafebabeull);
        REQUIRE(r.lo == 0xdeadbeefcafebabeull);
        REQUIRE(r.hi == 0);
    }
}

TEST_CASE("io::mul_u64: 0xffff.. * 2", "[u128][mul][carry]") {
    const io::u64 a = 0xffffffffffffffffull;
    const io::u64 b = 2ull;
    io::u128 r = io::mul_u64(a, b);
    REQUIRE(r.lo == 0xfffffffffffffffeull);
    REQUIRE(r.hi == 0x0000000000000001ull);
}

TEST_CASE("io::mul_u64: big hi (2^63 * 2^63 = 2^126)", "[u128][mul][hi]") {
    const io::u64 a = 0x8000000000000000ull;
    const io::u64 b = 0x8000000000000000ull;
    io::u128 r = io::mul_u64(a, b);
    REQUIRE(r.lo == 0x0000000000000000ull);
    REQUIRE(r.hi == 0x4000000000000000ull);
}

TEST_CASE("io::mul_u64: several deterministic regression pairs", "[u128][mul][reg]") {
    struct vec { io::u64 a, b, lo, hi; };
    const vec v[] = {
        // a*b computed offline / by reasoning:
        // 0xFFFFFFFF * 0xFFFFFFFF = 0xFFFFFFFE00000001
        { 0x00000000ffffffffull, 0x00000000ffffffffull, 0xfffffffe00000001ull, 0x0000000000000000ull },
        // (2^32) * (2^32) = 2^64
        { 0x0000000100000000ull, 0x0000000100000000ull, 0x0000000000000000ull, 0x0000000000000001ull },
        // 0x1_0000_0000 * 3 = 0x3_0000_0000
        { 0x0000000100000000ull, 0x0000000000000003ull, 0x0000000300000000ull, 0x0000000000000000ull },
    };

    for (auto& e : v) {
        io::u128 r = io::mul_u64(e.a, e.b);
        REQUIRE(r.lo == e.lo);
        REQUIRE(r.hi == e.hi);
    }
}


//TEST_CASE("hi::Key_t::map returns expected strings for a few keys", "[types][hi][key]") {
//    REQUIRE(                 hi::Key_t::map(hi::Key::A) == std::string("a"));
//    REQUIRE(                hi::Key_t::map(hi::Key::F1) == std::string("f1"));
//    REQUIRE(            hi::Key_t::map(hi::Key::Escape) == std::string("escape"));
//    REQUIRE(          hi::Key_t::map(hi::Key::__NONE__) == std::string("__NONE__"));
//    REQUIRE(   hi::Key_t::map(static_cast<hi::Key>(-1)) == std::string("unknown"));
//    REQUIRE(hi::Key_t::map(static_cast<hi::Key>(99999)) == std::string("unknown"));
//}
