#pragma once

#include <cstdint>

#if !defined(LMATH_FORCE_NO_SIMD)
#   if defined(_MSC_VER)
#       include <intrin.h>
#       if defined(__AVX__) || defined(__AVX2__)
#           include <immintrin.h>
#       endif
#   endif
#endif

#if defined(LMATH_FORCE_NO_SIMD)
#   define LMATH_HAS_SSE2 0
#   define LMATH_HAS_AVX  0
#   define LMATH_HAS_AVX2 0
#   define LMATH_HAS_NEON 0
#else
#   if defined(__x86_64__) || defined(_M_X64)
#       define LMATH_HAS_SSE2 1
#   elif defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#       define LMATH_HAS_SSE2 1
#   else
#       define LMATH_HAS_SSE2 0
#   endif

#   if defined(__AVX__)
#       define LMATH_HAS_AVX 1
#   else
#       define LMATH_HAS_AVX 0
#   endif

#   if defined(__AVX2__)
#       define LMATH_HAS_AVX2 1
#   else
#       define LMATH_HAS_AVX2 0
#   endif

#   if defined(__ARM_NEON) || defined(__aarch64__)
#       define LMATH_HAS_NEON 1
#   else
#       define LMATH_HAS_NEON 0
#   endif
#endif

#if !defined(LMATH_FORCE_NO_SIMD)
namespace lm {
namespace simd {

    enum class Level {
        none = 0,
        neon,
        sse2,
        avx,
        avx2,
    };

    const char* level_string(Level lvl) noexcept {
        switch (lvl)
        {
        case Level::sse2: return "SSE2";
        case Level::avx:  return "AVX";
        case Level::avx2: return "AVX2";
        case Level::neon: return "NEON";
        default: return "*none*";
        }
    }

#if defined(__x86_64__)  || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    static inline void cpuid(
        uint32_t leaf, uint32_t subleaf,
        uint32_t& eax, uint32_t& ebx,
        uint32_t& ecx, uint32_t& edx) noexcept {
#if defined(_MSC_VER)
        int regs[4];
        __cpuidex(regs, (int)leaf, (int)subleaf);
        eax = (uint32_t)regs[0];
        ebx = (uint32_t)regs[1];
        ecx = (uint32_t)regs[2];
        edx = (uint32_t)regs[3];
#else
        __asm__ volatile(
            "cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(leaf), "c"(subleaf)
            );
#endif
    }

    static inline uint64_t xgetbv(uint32_t index) noexcept
    {
#if defined(_MSC_VER)
        return ::_xgetbv(index);
#else
        uint32_t lo, hi;
        __asm__ volatile(
            "xgetbv"
            : "=a"(lo), "=d"(hi)
            : "c"(index)
            );
        return (uint64_t(hi) << 32) | lo;
#endif
    }
#endif // defined(__i386__) || defined(__x86_64__) || defined(_M_IX86)
    


    static inline Level runtime_level() noexcept {
#if defined(LMATH_FORCE_NO_SIMD)
        return Level::none;

#elif defined(__aarch64__) || defined(__ARM_NEON)
        return Level::neon;

#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)

        uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
        cpuid(1, 0, eax, ebx, ecx, edx);

        const bool hw_sse2 = (edx & (1u << 26)) != 0;
        const bool hw_avx = (ecx & (1u << 28)) != 0;
        const bool osxsave = (ecx & (1u << 27)) != 0;

#if LMATH_HAS_AVX || LMATH_HAS_AVX2
        if (hw_avx && osxsave) {
            const uint64_t xcr0 = xgetbv(0);
            const bool ymm_enabled = (xcr0 & 0x6) == 0x6;

            if (ymm_enabled) {
#if LMATH_HAS_AVX2
                cpuid(7, 0, eax, ebx, ecx, edx);
                if (ebx & (1u << 5))
                    return Level::avx2;
#endif
#if LMATH_HAS_AVX
                return Level::avx;
#endif
            }
        }
#endif

#if defined(__x86_64__) || defined(_M_X64)
        return Level::sse2;
#else
#   if LMATH_HAS_SSE2
        if (hw_sse2)
            return Level::sse2;
#   endif
        return Level::none;
#endif

#else
        return Level::none;
#endif
    } // runtime_level

    inline Level max_level() noexcept {
        static Level lvl = runtime_level();
        return lvl;
    }

} // namespace simd
} // namespace lm
#endif // LMATH_FORCE_NO_SIMD