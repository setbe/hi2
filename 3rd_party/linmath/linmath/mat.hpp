#pragma once

#include <cstddef>
#include <cstdint>

#include "detail/feature_detection.hpp"

#include "libc_integration.hpp"
#include "vec.hpp"

#if !defined(LMATH_FORCE_NO_SIMD)
#   if defined(__SSE2__)
#      include <emmintrin.h>
#   endif
#   if defined(__ARM_NEON)
#      include <arm_neon.h>
#   endif
#endif

namespace lm {

    // ============================================================
    // Matrix types (column-major, OpenGL-compatible)
    // ============================================================

    template<typename T, std::size_t C, std::size_t R>
    struct mat {
        LMATH_CONSTEXPR       vec<T,R>& operator[](std::size_t c)       noexcept { return col[c]; }
        LMATH_CONSTEXPR const vec<T,R>& operator[](std::size_t c) const noexcept { return col[c]; }

        vec<T,R> col[C]{};
    };

    // ============================================================
    // Aliases
    // ============================================================

    template<typename T> using mat3_of   = mat<T,3,3>;
    template<typename T> using mat4_of   = mat<T,4,4>;

    using mat3   = mat3_of<float>;
    using mat4   = mat4_of<float>;


    namespace detail {

        // ============================================================
        // mat4 × vec4
        // ============================================================
#if defined(__SSE2__)
        LMATH_FORCE_INLINE::lm::vec4 mat4_mul_vec_sse2(const ::lm::mat4& M,
            const ::lm::vec4& V) noexcept {
            const __m128 c0 = _mm_loadu_ps(M[0].data());
            const __m128 c1 = _mm_loadu_ps(M[1].data());
            const __m128 c2 = _mm_loadu_ps(M[2].data());
            const __m128 c3 = _mm_loadu_ps(M[3].data());

            const __m128 r =
                _mm_add_ps(
                    _mm_add_ps(_mm_mul_ps(c0, _mm_set1_ps(V[0])),
                        _mm_mul_ps(c1, _mm_set1_ps(V[1]))),
                    _mm_add_ps(_mm_mul_ps(c2, _mm_set1_ps(V[2])),
                        _mm_mul_ps(c3, _mm_set1_ps(V[3])))
                );

            ::lm::vec4 out{};
            _mm_storeu_ps(out.data(), r);
            return out;
        }
#endif


#if defined(__ARM_NEON)
        LMATH_FORCE_INLINE vec4 mat4_mul_vec_neon(const ::lm::mat4& M,
            const ::lm::vec4& V) noexcept {
            float32x4_t r =
                vmulq_n_f32(vld1q_f32(M[0].data()), V[0]);

            r = vmlaq_n_f32(r, vld1q_f32(M[1].data()), V[1]);
            r = vmlaq_n_f32(r, vld1q_f32(M[2].data()), V[2]);
            r = vmlaq_n_f32(r, vld1q_f32(M[3].data()), V[3]);

            ::lm::vec4 out{};
            vst1q_f32(out.v, r);
            return out;
        }
#endif

#if defined(__AVX__)
        LMATH_FORCE_INLINE::lm::vec4 mat4_mul_vec_avx(const ::lm::mat4& M,
            const ::lm::vec4& V) noexcept {
            const __m128 c0 = _mm_loadu_ps(M[0].data());
            const __m128 c1 = _mm_loadu_ps(M[1].data());
            const __m128 c2 = _mm_loadu_ps(M[2].data());
            const __m128 c3 = _mm_loadu_ps(M[3].data());

            // m01 = [c0 | c1]
            const __m256 m01 = _mm256_insertf128_ps(_mm256_castps128_ps256(c0), c1, 1);
            // m23 = [c2 | c3]
            const __m256 m23 = _mm256_insertf128_ps(_mm256_castps128_ps256(c2), c3, 1);

            // low lane = V[0], high lane = V[1]
            const __m256 b01 =
                _mm256_insertf128_ps(
                    _mm256_castps128_ps256(_mm_set1_ps(V[0])),
                    _mm_set1_ps(V[1]),
                    1
                );

            // low lane = V[2], high lane = V[3]
            const __m256 b23 =
                _mm256_insertf128_ps(
                    _mm256_castps128_ps256(_mm_set1_ps(V[2])),
                    _mm_set1_ps(V[3]),
                    1
                );

            // low  lane: c0*V[0] + c2*V[2]
            // high lane: c1*V[1] + c3*V[3]
            const __m256 p =
                _mm256_add_ps(
                    _mm256_mul_ps(m01, b01),
                    _mm256_mul_ps(m23, b23)
                );

            // result = low_lane + high_lane
            const __m128 lo = _mm256_castps256_ps128(p);
            const __m128 hi = _mm256_extractf128_ps(p, 1);
            const __m128 r = _mm_add_ps(lo, hi);

            ::lm::vec4 out{};
            _mm_storeu_ps(out.data(), r);
            return out;
        }
#endif


        // ============================================================
        // mat4 × mat4
        // ============================================================

#if defined(__SSE2__)
        LMATH_FORCE_INLINE::lm::mat4 mat4_mul_sse2(const ::lm::mat4& A,
            const ::lm::mat4& B) noexcept {
            ::lm::mat4 R{};

            const __m128 a0 = _mm_loadu_ps(A[0].data());
            const __m128 a1 = _mm_loadu_ps(A[1].data());
            const __m128 a2 = _mm_loadu_ps(A[2].data());
            const __m128 a3 = _mm_loadu_ps(A[3].data());

            {
                const __m128 b0 = _mm_set1_ps(B[0][0]);
                const __m128 b1 = _mm_set1_ps(B[0][1]);
                const __m128 b2 = _mm_set1_ps(B[0][2]);
                const __m128 b3 = _mm_set1_ps(B[0][3]);

                const __m128 r =
                    _mm_add_ps(
                        _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1)),
                        _mm_add_ps(_mm_mul_ps(a2, b2), _mm_mul_ps(a3, b3))
                    );

                _mm_storeu_ps(R[0].data(), r);
            }

            {
                const __m128 b0 = _mm_set1_ps(B[1][0]);
                const __m128 b1 = _mm_set1_ps(B[1][1]);
                const __m128 b2 = _mm_set1_ps(B[1][2]);
                const __m128 b3 = _mm_set1_ps(B[1][3]);

                const __m128 r =
                    _mm_add_ps(
                        _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1)),
                        _mm_add_ps(_mm_mul_ps(a2, b2), _mm_mul_ps(a3, b3))
                    );

                _mm_storeu_ps(R[1].data(), r);
            }

            {
                const __m128 b0 = _mm_set1_ps(B[2][0]);
                const __m128 b1 = _mm_set1_ps(B[2][1]);
                const __m128 b2 = _mm_set1_ps(B[2][2]);
                const __m128 b3 = _mm_set1_ps(B[2][3]);

                const __m128 r =
                    _mm_add_ps(
                        _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1)),
                        _mm_add_ps(_mm_mul_ps(a2, b2), _mm_mul_ps(a3, b3))
                    );

                _mm_storeu_ps(R[2].data(), r);
            }

            {
                const __m128 b0 = _mm_set1_ps(B[3][0]);
                const __m128 b1 = _mm_set1_ps(B[3][1]);
                const __m128 b2 = _mm_set1_ps(B[3][2]);
                const __m128 b3 = _mm_set1_ps(B[3][3]);

                const __m128 r =
                    _mm_add_ps(
                        _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1)),
                        _mm_add_ps(_mm_mul_ps(a2, b2), _mm_mul_ps(a3, b3))
                    );

                _mm_storeu_ps(R[3].data(), r);
            }

            return R;
        }
#endif

#if defined(__ARM_NEON)
        LMATH_FORCE_INLINE::lm::mat4 mat4_mul_neon(const ::lm::mat4& A,
            const ::lm::mat4& B) noexcept {
            ::lm::mat4 R{};

            const float32x4_t a0 = vld1q_f32(A[0].data());
            const float32x4_t a1 = vld1q_f32(A[1].data());
            const float32x4_t a2 = vld1q_f32(A[2].data());
            const float32x4_t a3 = vld1q_f32(A[3].data());

            {
                float32x4_t r = vmulq_n_f32(a0, B[0][0]);
                r = vmlaq_n_f32(r, a1, B[0][1]);
                r = vmlaq_n_f32(r, a2, B[0][2]);
                r = vmlaq_n_f32(r, a3, B[0][3]);
                vst1q_f32(R[0].data(), r);
            }

            {
                float32x4_t r = vmulq_n_f32(a0, B[1][0]);
                r = vmlaq_n_f32(r, a1, B[1][1]);
                r = vmlaq_n_f32(r, a2, B[1][2]);
                r = vmlaq_n_f32(r, a3, B[1][3]);
                vst1q_f32(R[1].data(), r);
            }

            {
                float32x4_t r = vmulq_n_f32(a0, B[2][0]);
                r = vmlaq_n_f32(r, a1, B[2][1]);
                r = vmlaq_n_f32(r, a2, B[2][2]);
                r = vmlaq_n_f32(r, a3, B[2][3]);
                vst1q_f32(R[2].data(), r);
            }

            {
                float32x4_t r = vmulq_n_f32(a0, B[3][0]);
                r = vmlaq_n_f32(r, a1, B[3][1]);
                r = vmlaq_n_f32(r, a2, B[3][2]);
                r = vmlaq_n_f32(r, a3, B[3][3]);
                vst1q_f32(R[3].data(), r);
            }

            return R;
        }
#endif

#if defined(__AVX__)
        LMATH_FORCE_INLINE::lm::mat4 mat4_mul_avx(const ::lm::mat4& A,
                                                  const ::lm::mat4& B) noexcept {
            ::lm::mat4 R{};
            const __m128 a0_128 = _mm_loadu_ps(A[0].data());
            const __m128 a1_128 = _mm_loadu_ps(A[1].data());
            const __m128 a2_128 = _mm_loadu_ps(A[2].data());
            const __m128 a3_128 = _mm_loadu_ps(A[3].data());

            // duplicate each A column into both 128-bit lanes:
            // [Acol | Acol]
            const __m256 a0 = _mm256_insertf128_ps(_mm256_castps128_ps256(a0_128), a0_128, 1);
            const __m256 a1 = _mm256_insertf128_ps(_mm256_castps128_ps256(a1_128), a1_128, 1);
            const __m256 a2 = _mm256_insertf128_ps(_mm256_castps128_ps256(a2_128), a2_128, 1);
            const __m256 a3 = _mm256_insertf128_ps(_mm256_castps128_ps256(a3_128), a3_128, 1);

            // helper: low lane = xxxx, high lane = yyyy
            auto lane_broadcast2 = [](float x, float y) noexcept -> __m256 {
                const __m128 lo = _mm_set1_ps(x);
                const __m128 hi = _mm_set1_ps(y);
                return _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1);
            };

            // columns 0 and 1
            {
                const __m256 b0 = lane_broadcast2(B[0][0], B[1][0]);
                const __m256 b1 = lane_broadcast2(B[0][1], B[1][1]);
                const __m256 b2 = lane_broadcast2(B[0][2], B[1][2]);
                const __m256 b3 = lane_broadcast2(B[0][3], B[1][3]);

                const __m256 r =
                    _mm256_add_ps(
                        _mm256_add_ps(_mm256_mul_ps(a0, b0), _mm256_mul_ps(a1, b1)),
                        _mm256_add_ps(_mm256_mul_ps(a2, b2), _mm256_mul_ps(a3, b3))
                    );

                _mm_storeu_ps(R[0].data(), _mm256_castps256_ps128(r));
                _mm_storeu_ps(R[1].data(), _mm256_extractf128_ps(r, 1));
            }

            // columns 2 and 3
            {
                const __m256 b0 = lane_broadcast2(B[2][0], B[3][0]);
                const __m256 b1 = lane_broadcast2(B[2][1], B[3][1]);
                const __m256 b2 = lane_broadcast2(B[2][2], B[3][2]);
                const __m256 b3 = lane_broadcast2(B[2][3], B[3][3]);

                const __m256 r =
                    _mm256_add_ps(
                        _mm256_add_ps(_mm256_mul_ps(a0, b0), _mm256_mul_ps(a1, b1)),
                        _mm256_add_ps(_mm256_mul_ps(a2, b2), _mm256_mul_ps(a3, b3))
                    );

                _mm_storeu_ps(R[2].data(), _mm256_castps256_ps128(r));
                _mm_storeu_ps(R[3].data(), _mm256_extractf128_ps(r, 1));
            }

            return R;
        }
#endif

    } // namespace detail

    // ============================================================
    // Identity
    // ============================================================

    LMATH_OUT mat4 mat4_identity() noexcept {
        return { {
            {1.f, 0.f, 0.f, 0.f},
            {0.f, 1.f, 0.f, 0.f},
            {0.f, 0.f, 1.f, 0.f},
            {0.f, 0.f, 0.f, 1.f}
        } };
    }
    
    /* fallback */template<typename T, std::size_t N>
    LMATH_OUT mat<T, N, N> mat_identity() noexcept {
        mat<T, N, N> M{};
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                M[i][j] = T(i == j);
        return M;
    }

    // ============================================================
    // Basic ops
    // ============================================================

    // --- add ---
    LMATH_OUT mat3 mat_add(const mat3& A, const mat3& B) noexcept {
        return { {
            {A[0][0] + B[0][0], A[0][1] + B[0][1], A[0][2] + B[0][2]},
            {A[1][0] + B[1][0], A[1][1] + B[1][1], A[1][2] + B[1][2]},
            {A[2][0] + B[2][0], A[2][1] + B[2][1], A[2][2] + B[2][2]}
        } };
    }
    LMATH_OUT mat4 mat_add(const mat4& A, const mat4& B) noexcept {
        return { {
            {A[0][0] + B[0][0], A[0][1] + B[0][1], A[0][2] + B[0][2], A[0][3] + B[0][3]},
            {A[1][0] + B[1][0], A[1][1] + B[1][1], A[1][2] + B[1][2], A[1][3] + B[1][3]},
            {A[2][0] + B[2][0], A[2][1] + B[2][1], A[2][2] + B[2][2], A[2][3] + B[2][3]},
            {A[3][0] + B[3][0], A[3][1] + B[3][1], A[3][2] + B[3][2], A[3][3] + B[3][3]}
        } };
    }
    
    /* fallback */template<typename T, std::size_t C, std::size_t R> LMATH_OUT mat<T,C,R>
    mat_add(const mat<T,C,R>& A, const mat<T,C,R>& B) noexcept {
        mat<T,C,R> M{};
        for (int c=0; c<C; ++c)
            M[c] = A[c] + B[c];
        return M;
    }

    // --- sub ---
    LMATH_OUT mat3 mat_sub(const mat3& A, const mat3& B) noexcept {
        return { {
            {A[0][0] - B[0][0], A[0][1] - B[0][1], A[0][2] - B[0][2]},
            {A[1][0] - B[1][0], A[1][1] - B[1][1], A[1][2] - B[1][2]},
            {A[2][0] - B[2][0], A[2][1] - B[2][1], A[2][2] - B[2][2]}
        } };
    }
    LMATH_OUT mat4 mat_sub(const mat4& A, const mat4& B) noexcept {
        return { {
            {A[0][0] - B[0][0], A[0][1] - B[0][1], A[0][2] - B[0][2], A[0][3] - B[0][3]},
            {A[1][0] - B[1][0], A[1][1] - B[1][1], A[1][2] - B[1][2], A[1][3] - B[1][3]},
            {A[2][0] - B[2][0], A[2][1] - B[2][1], A[2][2] - B[2][2], A[2][3] - B[2][3]},
            {A[3][0] - B[3][0], A[3][1] - B[3][1], A[3][2] - B[3][2], A[3][3] - B[3][3]}
        } };
    }

    /* fallback */template<typename T, std::size_t C, std::size_t R> LMATH_OUT mat<T,C,R>
    mat_sub(const mat<T,C,R>& A, const mat<T,C,R>& B) noexcept {
        mat<T,C,R> M{};
        for (int c=0; c<C; ++c)
            M[c] = A[c] - B[c];
        return M;
    }

    // --- scale ---
    LMATH_OUT mat3 mat_scale(const mat3& A, float S) noexcept {
        return { {
            {A[0][0] * S, A[0][1] * S, A[0][2] * S},
            {A[1][0] * S, A[1][1] * S, A[1][2] * S},
            {A[2][0] * S, A[2][1] * S, A[2][2] * S}
        } };
    }
    LMATH_OUT mat4 mat_scale(const mat4& A, float S) noexcept {
        return { {
            {A[0][0] * S, A[0][1] * S, A[0][2] * S, A[0][3] * S},
            {A[1][0] * S, A[1][1] * S, A[1][2] * S, A[1][3] * S},
            {A[2][0] * S, A[2][1] * S, A[2][2] * S, A[2][3] * S},
            {A[3][0] * S, A[3][1] * S, A[3][2] * S, A[3][3] * S}
        } };
    }

    /* fallback */template<typename T, std::size_t C, std::size_t R> LMATH_OUT mat<T,C,R>
    mat_scale(const mat<T,C,R>& A, T S) noexcept {
        mat<T,C,R> M{};
        for (int c=0; c<C; ++c)
            M[c] = A[c] * S;
        return M;
    }

    // ============================================================
    // Multiplication
    // ============================================================

    // --- mat4 x mat4 ---
    
    /* M4*M4 scalar */LMATH_OUT mat4
    mat4_mul_scalar(const mat4& A, const mat4& B) noexcept {
        mat4 R{};
        for (int c = 0; c < 4; ++c) {
            const float b0 = B[c][0];
            const float b1 = B[c][1];
            const float b2 = B[c][2];
            const float b3 = B[c][3];

            R[c][0] = A[0][0] * b0 + A[1][0] * b1 + A[2][0] * b2 + A[3][0] * b3;
            R[c][1] = A[0][1] * b0 + A[1][1] * b1 + A[2][1] * b2 + A[3][1] * b3;
            R[c][2] = A[0][2] * b0 + A[1][2] * b1 + A[2][2] * b2 + A[3][2] * b3;
            R[c][3] = A[0][3] * b0 + A[1][3] * b1 + A[2][3] * b2 + A[3][3] * b3;
    }
        return R;
    }

    /* M4*M4 SIMD */LMATH_NO_DISCARD inline mat4
    mat4_mul(const mat4& A, const mat4& B) noexcept {
#if defined(LMATH_FORCE_NO_SIMD)
        return mat4_mul_scalar(A, B);
#else
        switch (simd::max_level()) {
#if defined(__ARM_NEON)
        case simd::Level::neon:
            return detail::mat4_mul_neon(A, B);
#endif
#if defined(__AVX2__)
        case simd::Level::avx2:
#endif
#if defined(__AVX__)
        case simd::Level::avx:
            return detail::mat4_mul_avx(A, B);
#endif
#if defined(__SSE2__)
        case simd::Level::sse2:
            return detail::mat4_mul_sse2(A, B);
#endif
        default:
            return mat4_mul_scalar(A, B);
        } // switch
#endif // LMATH_FORCE_NO_SIMD
    } // mat4_mul

    /* M4*M4 fallback */template<typename T, std::size_t N> LMATH_OUT mat<T,N,N>
    mat_mul(const mat<T,N,N>& A, const mat<T,N,N>& B) noexcept {
        mat<T, N, N> R{};
        for (int c = 0; c < N; ++c)
            for (int r = 0; r < N; ++r) {
                T sum{};  for (int k = 0; k < N; ++k)  sum += A[k][r] * B[c][k];
                R[c][r] = sum;
    }
        return R;
    }

    // --- mat4 x vec4 ---

    /* M4*V4 scalar */LMATH_OUT vec4
    mat4_mul_vec_scalar(const mat4& M, const vec4& V) noexcept {
        const float x = V[0];
        const float y = V[1];
        const float z = V[2];
        const float w = V[3];
        return {
            M[0][0] * x + M[1][0] * y + M[2][0] * z + M[3][0] * w,
            M[0][1] * x + M[1][1] * y + M[2][1] * z + M[3][1] * w,
            M[0][2] * x + M[1][2] * y + M[2][2] * z + M[3][2] * w,
            M[0][3] * x + M[1][3] * y + M[2][3] * z + M[3][3] * w
        };
    }

    /* M4*V4 SIMD */LMATH_NO_DISCARD inline vec4
    mat4_mul_vec(const mat4& M, const vec4& V) noexcept {
#if defined(LMATH_FORCE_NO_SIMD)
        return mat4_mul_vec_scalar(M, V); // m4v4 fallback-specialization
#else
        switch (simd::max_level()) {
#if defined(__ARM_NEON)
        case simd::Level::neon:
            return detail::mat4_mul_vec_neon(M, V);
#endif
#if defined(__AVX__)
        case simd::Level::avx:
#if defined(__AVX2__)
        case simd::Level::avx2:
#endif
            return detail::mat4_mul_vec_avx(M, V);
#endif
#if defined(__SSE2__)
        case simd::Level::sse2:
            return detail::mat4_mul_vec_sse2(M, V);
#endif
        default:
            return mat4_mul_vec_scalar(M, V); // m4v4 fallback-specialization
        } // switch
#endif // LMATH_FORCE_NO_SIMD
    } // mat_mul_vec

    /* M4*V4 fallback */template<typename T, std::size_t N>
    LMATH_OUT vec<T,N> mat_mul_vec(const mat<T,N,N>& M, const vec<T,N>& V) noexcept {
        vec<T,N> R{};
        for (int r=0; r<N; ++r) {
            T s{};
            for (int c=0; c<N; ++c)
                s += M[c][r] * V[c];
            R[r] = s;
        }
        return R;
    }

    // ============================================================
    // Transpose
    // ============================================================
    LMATH_OUT mat3 mat_transpose(const mat3& M) noexcept {
        return { {
            {M[0][0], M[1][0], M[2][0]},
            {M[0][1], M[1][1], M[2][1]},
            {M[0][2], M[1][2], M[2][2]}
        } };
    }
    LMATH_OUT mat4 mat_transpose(const mat4& M) noexcept {
        return { {
            {M[0][0], M[1][0], M[2][0], M[3][0]},
            {M[0][1], M[1][1], M[2][1], M[3][1]},
            {M[0][2], M[1][2], M[2][2], M[3][2]},
            {M[0][3], M[1][3], M[2][3], M[3][3]}
        } };
    }
    
    /* fallback */template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT mat<T,R,C> mat_transpose(const mat<T,C,R>& M) noexcept {
        mat<T,R,C> Rm{};
        for (int c=0; c<C; ++c)
            for (int r=0; r<R; ++r)
                Rm[r][c] = M[c][r];
        return Rm;
    }

    // ============================================================
    // mat3 transforms
    // ============================================================
    LMATH_OUT mat3 mat3_translate(float x, float y) noexcept {
        return { {
            {1.f, 0.f, 0.f},
            {0.f, 1.f, 0.f},
            { x,   y,  1.f}
        } };
    }
    LMATH_OUT mat3 mat3_scale(float x, float y) noexcept {
        mat3 M{};
        M[0][0] = x;
        M[1][1] = y;
        M[2][2] = 1.f;
        return M;
    }
    LMATH_OUT mat3 mat3_rotate(float a) noexcept {
        float s = ::lm::sinf(a);
        float c = ::lm::cosf(a);

        mat3 M{};
        M[0][0] =  c; M[1][0] = -s;
        M[0][1] =  s; M[1][1] =  c;
        M[2][2] = 1.f;
        return M;
    }

    // ============================================================
    // mat4 transforms
    // ============================================================
    LMATH_OUT mat4 mat4_translate(float x, float y, float z) noexcept {
        return { {
            {1.f, 0.f, 0.f, 0.f},
            {0.f, 1.f, 0.f, 0.f},
            {0.f, 0.f, 1.f, 0.f},
            {x,   y,   z,   1.f}
        } };
    }
    LMATH_OUT mat4 mat4_scale(float x, float y, float z) noexcept {
        return { {
            {x,   0.f, 0.f, 0.f},
            {0.f, y,   0.f, 0.f},
            {0.f, 0.f, z,   0.f},
            {0.f, 0.f, 0.f, 1.f}
        } };
    }

    LMATH_OUT mat4 mat4_rotate_x(float a) noexcept {
        float s = ::lm::sinf(a);
        float c = ::lm::cosf(a);

        return { {
            {1.f, 0.f, 0.f, 0.f},
            {0.f, c,   s,   0.f},
            {0.f, -s,  c,   0.f},
            {0.f, 0.f, 0.f, 1.f}
        } };
    }
    LMATH_OUT mat4 mat4_rotate_y(float a) noexcept {
        float s = ::lm::sinf(a);
        float c = ::lm::cosf(a);

        return { {
            { c,   0.f, -s,  0.f},
            {0.f,  1.f, 0.f, 0.f},
            { s,   0.f,  c,  0.f},
            {0.f,  0.f, 0.f, 1.f}
        } };
    }
    LMATH_OUT mat4 mat4_rotate_z(float a) noexcept {
        float s = ::lm::sinf(a);
        float c = ::lm::cosf(a);

        return { {
            { c,   s,   0.f, 0.f},
            {-s,   c,   0.f, 0.f},
            {0.f,  0.f, 1.f, 0.f},
            {0.f,  0.f, 0.f, 1.f}
        } };
    }

    // ============================================================
    // Projection (OpenGL ES compatible)
    // ============================================================

    LMATH_OUT mat4 mat4_perspective(float fov_y,
                                    float aspect,
                                    float n,
                                    float f) noexcept {
        float a = 1.f / ::lm::tanf(fov_y * 0.5f);

        mat4 M{};
        M[0][0] = a / aspect;
        M[1][1] = a;
        M[2][2] = -(f+n)/(f-n);
        M[2][3] = -1.f;
        M[3][2] = -(2.f*f*n)/(f-n);
        return M;
    }
    LMATH_OUT mat4 mat4_ortho(float l, float r,
                              float b, float t,
                              float n, float f) noexcept {
        mat4 M{};
        M[0][0] =  2.f/(r-l);
        M[1][1] =  2.f/(t-b);
        M[2][2] = -2.f/(f-n);
        M[3][0] = -(r+l)/(r-l);
        M[3][1] = -(t+b)/(t-b);
        M[3][2] = -(f+n)/(f-n);
        M[3][3] =  1.f;
        return M;
    }

    // ============================================================
    // Look At
    // ============================================================
    
    LMATH_OUT mat4 mat4_look_at(const vec3& eye,
        const vec3& center,
        const vec3& up) noexcept
    {
        const float ex = eye[0], ey = eye[1], ez = eye[2];
        const float upx = up[0], upy = up[1], upz = up[2];

        float fx = center[0] - ex;
        float fy = center[1] - ey;
        float fz = center[2] - ez;

        float inv_len = rsqrtf_pos(fx * fx + fy * fy + fz * fz);
        fx *= inv_len;
        fy *= inv_len;
        fz *= inv_len;

        float sx = fy * upz - fz * upy;
        float sy = fz * upx - fx * upz;
        float sz = fx * upy - fy * upx;

        inv_len = rsqrtf_pos(sx * sx + sy * sy + sz * sz);
        sx *= inv_len;
        sy *= inv_len;
        sz *= inv_len;

        const float ux = sy * fz - sz * fy;
        const float uy = sz * fx - sx * fz;
        const float uz = sx * fy - sy * fx;

        return { {
            { sx,  ux, -fx, 0.f },
            { sy,  uy, -fy, 0.f },
            { sz,  uz, -fz, 0.f },
            {
                -(sx * ex + sy * ey + sz * ez),
                -(ux * ex + uy * ey + uz * ez),
                 (fx * ex + fy * ey + fz * ez),
                 1.f
            }
        } };
    }
    
    // ============================================================
    // overloaded operators (specialized hot paths)
    // ============================================================
    
    // +
    
    LMATH_OUT mat3 operator+(const mat3& A, const mat3& B) noexcept {
        return { {
            {A[0][0] + B[0][0], A[0][1] + B[0][1], A[0][2] + B[0][2]},
            {A[1][0] + B[1][0], A[1][1] + B[1][1], A[1][2] + B[1][2]},
            {A[2][0] + B[2][0], A[2][1] + B[2][1], A[2][2] + B[2][2]}
        } };
    }
    LMATH_OUT mat4 operator+(const mat4& A, const mat4& B) noexcept {
        return { {
            {A[0][0] + B[0][0], A[0][1] + B[0][1], A[0][2] + B[0][2], A[0][3] + B[0][3]},
            {A[1][0] + B[1][0], A[1][1] + B[1][1], A[1][2] + B[1][2], A[1][3] + B[1][3]},
            {A[2][0] + B[2][0], A[2][1] + B[2][1], A[2][2] + B[2][2], A[2][3] + B[2][3]},
            {A[3][0] + B[3][0], A[3][1] + B[3][1], A[3][2] + B[3][2], A[3][3] + B[3][3]}
        } };
    }
    
    /* fallback */template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT mat<T, C, R> operator+(const mat<T, C, R>& A, const mat<T, C, R>& B) noexcept {
        return mat_add(A, B);
    }

    // -

    LMATH_OUT mat3 operator-(const mat3& A, const mat3& B) noexcept {
        return { {
            {A[0][0] - B[0][0], A[0][1] - B[0][1], A[0][2] - B[0][2]},
            {A[1][0] - B[1][0], A[1][1] - B[1][1], A[1][2] - B[1][2]},
            {A[2][0] - B[2][0], A[2][1] - B[2][1], A[2][2] - B[2][2]}
        } };
    }
    LMATH_OUT mat4 operator-(const mat4& A, const mat4& B) noexcept {
        return { {
            {A[0][0] - B[0][0], A[0][1] - B[0][1], A[0][2] - B[0][2], A[0][3] - B[0][3]},
            {A[1][0] - B[1][0], A[1][1] - B[1][1], A[1][2] - B[1][2], A[1][3] - B[1][3]},
            {A[2][0] - B[2][0], A[2][1] - B[2][1], A[2][2] - B[2][2], A[2][3] - B[2][3]},
            {A[3][0] - B[3][0], A[3][1] - B[3][1], A[3][2] - B[3][2], A[3][3] - B[3][3]}
        } };
    }
    
    /* fallback */template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT mat<T, C, R> operator-(const mat<T, C, R>& A,
        const mat<T, C, R>& B) noexcept {
        return mat_sub(A, B);
    }

    // mat * scalar

    LMATH_OUT mat3 operator*(const mat3& A, float S) noexcept {
        return { {
            {A[0][0] * S, A[0][1] * S, A[0][2] * S},
            {A[1][0] * S, A[1][1] * S, A[1][2] * S},
            {A[2][0] * S, A[2][1] * S, A[2][2] * S}
        } };
    }
    LMATH_OUT mat4 operator*(const mat4& A, float S) noexcept {
        return { {
            {A[0][0] * S, A[0][1] * S, A[0][2] * S, A[0][3] * S},
            {A[1][0] * S, A[1][1] * S, A[1][2] * S, A[1][3] * S},
            {A[2][0] * S, A[2][1] * S, A[2][2] * S, A[2][3] * S},
            {A[3][0] * S, A[3][1] * S, A[3][2] * S, A[3][3] * S}
        } };
    }

    /* fallback */template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT mat<T, C, R> operator*(const mat<T, C, R>& A, T S) noexcept {
        return mat_scale(A, S);
    }

    // scalar * mat

    LMATH_OUT mat3 operator*(float S, const mat3& A) noexcept { return A * S; }
    LMATH_OUT mat4 operator*(float S, const mat4& A) noexcept { return A * S; }

    /* fallback */template<typename T, std::size_t C, std::size_t R>
    LMATH_CONSTEXPR mat<T, C, R> operator*(T S, const mat<T, C, R>& A) noexcept {
        return mat_scale(A, S);
    }

    // mat * mat

    LMATH_OUT mat3 operator*(const mat3& A, const mat3& B) noexcept { return mat_mul(A, B); }
    LMATH_OUT mat4 operator*(const mat4& A, const mat4& B) noexcept { return mat4_mul(A, B);}

    /* fallback */template<typename T, std::size_t C1, std::size_t R1, std::size_t C2>
    LMATH_OUT mat<T, C2, R1> operator*(const mat<T, C1, R1>& A,
        const mat<T, C2, C1>& B) noexcept {
        return mat_mul(A, B);
    }

    // mat * vec

    LMATH_OUT vec3 operator*(const mat3& M, const vec3& V) noexcept {
        return {
            M[0][0] * V[0] + M[1][0] * V[1] + M[2][0] * V[2],
            M[0][1] * V[0] + M[1][1] * V[1] + M[2][1] * V[2],
            M[0][2] * V[0] + M[1][2] * V[1] + M[2][2] * V[2]
        };
    }
    LMATH_OUT vec4 operator*(const mat4& M, const vec4& V) noexcept {
        return mat4_mul_vec(M, V);
    }
    
    /* fallback */template<typename T, std::size_t N>
    LMATH_CONSTEXPR vec<T, N> operator*(const mat<T, N, N>& M,
        const vec<T, N>& V) noexcept {
        return mat_mul_vec(M, V);
    }

    // vec * mat

    LMATH_OUT vec3 operator*(const vec3& V, const mat3& M) noexcept {
        return {
            V[0] * M[0][0] + V[1] * M[0][1] + V[2] * M[0][2],
            V[0] * M[1][0] + V[1] * M[1][1] + V[2] * M[1][2],
            V[0] * M[2][0] + V[1] * M[2][1] + V[2] * M[2][2]
        };
    }
    LMATH_OUT vec4 operator*(const vec4& V, const mat4& M) noexcept {
        return {
            V[0] * M[0][0] + V[1] * M[0][1] + V[2] * M[0][2] + V[3] * M[0][3],
            V[0] * M[1][0] + V[1] * M[1][1] + V[2] * M[1][2] + V[3] * M[1][3],
            V[0] * M[2][0] + V[1] * M[2][1] + V[2] * M[2][2] + V[3] * M[2][3],
            V[0] * M[3][0] + V[1] * M[3][1] + V[2] * M[3][2] + V[3] * M[3][3]
        };
    }
    
    /* fallback */template<typename T, std::size_t N>
    LMATH_CONSTEXPR vec<T, N> operator*(const vec<T, N>& V,
        const mat<T, N, N>& M) noexcept {
        vec<T, N> res{};
        for (int c = 0; c < N; ++c) {
            T s{};
            for (int k = 0; k < N; ++k)
                s += V[k] * M[c][k];
            res[c] = s;
        }
        return res;
    }

    // /

    LMATH_OUT mat3 operator/(const mat3& A, float s) noexcept {
        const float inv = 1.f / s;
        return A * inv;
    }
    LMATH_OUT mat4 operator/(const mat4& A, float s) noexcept {
        const float inv = 1.f / s;
        return A * inv;
    }

    /* fallback */template<typename T, std::size_t C, std::size_t R>
    LMATH_CONSTEXPR mat<T, C, R> operator/(const mat<T, C, R>& A, T s) noexcept {
        return mat_scale(A, T(1) / s);
    }

    // *=

    LMATH_OUT mat3& operator*=(mat3& A, float s) noexcept {
        A[0][0] *= s; A[0][1] *= s; A[0][2] *= s;
        A[1][0] *= s; A[1][1] *= s; A[1][2] *= s;
        A[2][0] *= s; A[2][1] *= s; A[2][2] *= s;
        return A;
    }
    LMATH_OUT mat4& operator*=(mat4& A, float s) noexcept {
        A[0][0] *= s; A[0][1] *= s; A[0][2] *= s; A[0][3] *= s;
        A[1][0] *= s; A[1][1] *= s; A[1][2] *= s; A[1][3] *= s;
        A[2][0] *= s; A[2][1] *= s; A[2][2] *= s; A[2][3] *= s;
        A[3][0] *= s; A[3][1] *= s; A[3][2] *= s; A[3][3] *= s;
        return A;
    }

    /* fallback */template<typename T, std::size_t C, std::size_t R>
    LMATH_CONSTEXPR mat<T, C, R>& operator*=(mat<T, C, R>& A, T s) noexcept {
        for (int c = 0; c < C; ++c)
            A[c] *= s;
        return A;
    }

    // ==

    LMATH_OUT bool operator==(const mat3& a, const mat3& b) noexcept {
        return
            a[0][0] == b[0][0] && a[0][1] == b[0][1] && a[0][2] == b[0][2] &&
            a[1][0] == b[1][0] && a[1][1] == b[1][1] && a[1][2] == b[1][2] &&
            a[2][0] == b[2][0] && a[2][1] == b[2][1] && a[2][2] == b[2][2];
    }
    LMATH_OUT bool operator==(const mat4& a, const mat4& b) noexcept {
        return
            a[0][0] == b[0][0] && a[0][1] == b[0][1] && a[0][2] == b[0][2] && a[0][3] == b[0][3] &&
            a[1][0] == b[1][0] && a[1][1] == b[1][1] && a[1][2] == b[1][2] && a[1][3] == b[1][3] &&
            a[2][0] == b[2][0] && a[2][1] == b[2][1] && a[2][2] == b[2][2] && a[2][3] == b[2][3] &&
            a[3][0] == b[3][0] && a[3][1] == b[3][1] && a[3][2] == b[3][2] && a[3][3] == b[3][3];
    }

    /* fallback */template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT bool operator==(const mat<T,C,R>& a, const mat<T,C,R>& b) noexcept {
        for (int c=0; c<C; ++c)
            for (int r=0; r<R; ++r)
                if (a[c][r] != b[c][r])
                    return false;
        return true;
    }

    // !=

    template<typename T, std::size_t C, std::size_t R>
    LMATH_OUT bool operator!=(const mat<T,C,R>& a, const mat<T,C,R>& b) noexcept {
        return !(a == b);
    }

} // namespace lm
