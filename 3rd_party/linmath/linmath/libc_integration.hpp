#pragma once
#include <stddef.h> // size_t
#include <stdint.h> // uint32_t

#include "detail/feature_detection.hpp"
#include "detail/simd_integration.hpp"

namespace lm {
    LMATH_CONSTEXPR_VAR float PI = 3.14159265359f;
    LMATH_CONSTEXPR_VAR float PI_HALF = 1.57079632679f;
    LMATH_CONSTEXPR_VAR float PI_DOUBLE = 6.28318530718f;

    LMATH_OUT float radians(float degrees) noexcept {
        return degrees * PI / 180.f;
    } // radians

    // ------------------------ Functions decl --------------------------------
    LMATH_OUT float sinf(float X) noexcept;
    LMATH_OUT float cosf(float X) noexcept;
    LMATH_OUT float tanf(float X) noexcept;
    LMATH_OUT float sqrtf(float X) noexcept; // Only one iteration. ~0.175 ulp.
    LMATH_OUT float floorf(float X) noexcept;


    // ------------------------ Functions impl --------------------------------
    LMATH_OUT float sinf(float X) noexcept {
        bool flip = false;
        const float x2 = X*X;

        // X to [0, 2pi)
        while (X >= PI_DOUBLE) X -= PI_DOUBLE;
        while (X < 0.f)        X += PI_DOUBLE;

        // [-pi/2, pi/2]
        if (X > PI) {
            X -= PI;
            flip = true;
        }
        if (X > PI_HALF) X = PI-X;

        // [-pi/2, pi/2]
        float result = X * (1.f - x2/6.f + x2*x2/120.0f);
        return flip ? -result : result;
    } // sinf


    LMATH_OUT float cosf(float X) noexcept { return sinf(X + PI_HALF); }


    LMATH_OUT float tanf(float X) noexcept {
        const float x2 = X * X;
        // [-pi, pi)
        while (X >  PI) X -= PI_DOUBLE;
        while (X < -PI) X += PI_DOUBLE;

        // tanf(X) ~= X + X^3/3 + 2x^5/15 + 17x^7/315
        return X + X*x2 *  (  1.f/3.f + x2*(2.f/15.f + x2*(17.f/315.f))  );
    } // tanf

    LMATH_FORCE_INLINE
    LMATH_NO_DISCARD float rsqrtf_scalar(float X) noexcept {
        if (X <= 0.f) return 0.f;
        const float x_half = 0.5f * X;
        union {
            float f;
            uint32_t i;
        } u{ X };
        u.i = 0x5f3759dfu - (u.i >> 1);

        float y = u.f;
        y = y * (1.5f - x_half * y * y);
        y = y * (1.5f - x_half * y * y);
        return y;
    }


    LMATH_FORCE_INLINE
    LMATH_NO_DISCARD float rsqrtf(float x) noexcept {
        if (x <= 0.f) return 0.f;
#if !defined(__SSE__)
        return rsqrtf_scalar(x);
#else
        switch (simd::max_level()) {
        case simd::Level::sse2:
        case simd::Level::avx:
        case simd::Level::avx2: {
            __m128 v = _mm_set_ss(x);
            __m128 y = _mm_rsqrt_ss(v);

            // one Newton refinement
            const __m128 half = _mm_set_ss(0.5f);
            const __m128 three_halfs = _mm_set_ss(1.5f);

            __m128 y2 = _mm_mul_ss(y, y);
            __m128 xy2 = _mm_mul_ss(v, y2);
            __m128 term = _mm_sub_ss(three_halfs, _mm_mul_ss(half, xy2));
            y = _mm_mul_ss(y, term);

            return _mm_cvtss_f32(y);
        }
        default: return rsqrtf_scalar(x);
        }
    }
#endif
    }

    LMATH_FORCE_INLINE
    LMATH_NO_DISCARD float rsqrtf_pos(float x) noexcept {
    #if !defined(__SSE__)
        return rsqrtf_scalar(x);
    #else
        __m128 v = _mm_set_ss(x);
        __m128 y = _mm_rsqrt_ss(v);
        return _mm_cvtss_f32(y);
    #endif
    }

    LMATH_FORCE_INLINE
    LMATH_NO_DISCARD float sqrtf(float X) noexcept {
        return X <= 0.f ? 0.f : X * rsqrtf_pos(X);
    }


    LMATH_OUT float floorf(float X) noexcept {
        int i = (int)X;
        if (X < 0.0f && X != static_cast<float>(i)) --i;
        return static_cast<float>(i);
    } // floorf
} // namespace lm