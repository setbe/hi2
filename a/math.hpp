#pragma once

#include "config.hpp"
#include "detail/arch.hpp"
#include "types.hpp"

#if A_COMPILER_MSVC && A_ARCH_X86_64
extern "C" unsigned __int64 _umul128(unsigned __int64, unsigned __int64, unsigned __int64*);
#endif

namespace a {
A_NODISCARD static inline float ceil(float x) noexcept {
    // NaN passthrough.
    if (!(x == x)) {
        return x;
    }

    // For |x| >= 2^24 every float32 value is already integral.
    if (x >= 16777216.0f || x <= -16777216.0f) {
        return x;
    }

    const int i = static_cast<int>(x); // trunc toward zero
    return (x > static_cast<float>(i)) ? static_cast<float>(i + 1) : static_cast<float>(i);
}

A_NODISCARD static inline u128 add_u128(u128 a, u128 b) noexcept {
    u128 r{};
    r.lo = a.lo + b.lo;
    r.hi = a.hi + b.hi + (r.lo < a.lo);
    return r;
}

A_NODISCARD static inline u128 add_u128_u64(u128 a, u64 b) noexcept {
    u128 r{};
    r.lo = a.lo + b;
    r.hi = a.hi + (r.lo < a.lo);
    return r;
}

A_NODISCARD static inline u64 mul_u32(u32 a, u32 b) noexcept {
    return static_cast<u64>(a) * static_cast<u64>(b);
}

namespace detail {
template<typename ArchTag>
struct mul_u64_policy {
    static A_FORCE_INLINE u128 run(u64 a, u64 b) noexcept {
        const u32 a0 = static_cast<u32>(a);
        const u32 a1 = static_cast<u32>(a >> 32);
        const u32 b0 = static_cast<u32>(b);
        const u32 b1 = static_cast<u32>(b >> 32);

        const u64 p00 = static_cast<u64>(a0) * static_cast<u64>(b0);
        const u64 p01 = static_cast<u64>(a0) * static_cast<u64>(b1);
        const u64 p10 = static_cast<u64>(a1) * static_cast<u64>(b0);
        const u64 p11 = static_cast<u64>(a1) * static_cast<u64>(b1);

        u64 lo = p00;
        u64 hi = p11 + (p01 >> 32) + (p10 >> 32);

        const u64 add1 = (p01 << 32);
        const u64 before1 = lo;
        lo = before1 + add1;
        hi += (lo < before1);

        const u64 add2 = (p10 << 32);
        const u64 before2 = lo;
        lo = before2 + add2;
        hi += (lo < before2);

        return {lo, hi};
    }
};

template<>
struct mul_u64_policy<arch_x86_64> {
    static A_FORCE_INLINE u128 run(u64 a, u64 b) noexcept {
#if A_COMPILER_MSVC && A_ARCH_X86_64
        u128 r{};
        r.lo = _umul128(a, b, &r.hi);
        return r;
#else
        return mul_u64_policy<arch_generic>::run(a, b);
#endif
    }
};

template<typename ArchTag>
A_FORCE_INLINE u128 mul_u64_impl(u64 a, u64 b) noexcept {
    return mul_u64_policy<ArchTag>::run(a, b);
}
} // namespace detail

A_NODISCARD static inline u128 mul_u64(u64 a, u64 b) noexcept {
#if A_HAS_INT128
    unsigned __int128 p = static_cast<unsigned __int128>(a) * static_cast<unsigned __int128>(b);
    return {static_cast<u64>(p), static_cast<u64>(p >> 64)};
#else
    return detail::mul_u64_impl<detail::arch_default>(a, b);
#endif
}

A_NODISCARD static inline u32 div_u64_u32(u64 n, u32 d, u32* rem = nullptr) noexcept {
    if (d == 0) {
        if (rem != nullptr) {
            *rem = 0;
        }
        return 0;
    }
    u64 q = 0;
    u32 r = 0;
    for (int i = 63; i >= 0; --i) {
        r = static_cast<u32>((r << 1) | static_cast<u32>((n >> i) & 1ull));
        if (r >= d) {
            r -= d;
            q |= (1ull << i);
        }
    }
    if (rem != nullptr) {
        *rem = r;
    }
    return static_cast<u32>(q);
}

A_NODISCARD static inline u8 f2u8(float x) noexcept {
    if (x < 0.0f) {
        x = 0.0f;
    }
    if (x > 1.0f) {
        x = 1.0f;
    }

    const float y = x * 255.0f + 0.5f;
    int iy = static_cast<int>(y);
    if (iy < 0) {
        iy = 0;
    }
    if (iy > 255) {
        iy = 255;
    }
    return static_cast<u8>(iy);
}
} // namespace a
