#pragma once

#include <cstddef>
#include <cstdint>

#include "detail/feature_detection.hpp"
#include "libc_integration.hpp"
#include "vec.hpp"
#include "mat.hpp"

namespace lm {

    // ============================================================
    // Quaternion type
    // layout: (x, y, z, w)
    // ============================================================

    template<typename T>
    struct quat_of {
        vec<T,3> v{}; // xyz
        T        w{}; // w

        LMATH_OUT T& operator[](std::size_t i)       noexcept { return i<3 ? v[i] : w; }
        LMATH_OUT T  operator[](std::size_t i) const noexcept { return i<3 ? v[i] : w; }
    };

    using quat = quat_of<float>;

    // ============================================================
    // Identity
    // ============================================================

    template<typename T = float >
    LMATH_OUT quat_of<T> quat_identity() noexcept {
        return { vec<T,3>{}, T(1) };
    }

    // ============================================================
    // Basic ops
    // ============================================================

    template<typename T>
    LMATH_OUT quat_of<T> quat_add(const quat_of<T>& A,
                                  const quat_of<T>& B) noexcept {
        return { A.v + B.v,
                 A.w + B.w };
    }

    template<typename T>
    LMATH_OUT quat_of<T> quat_sub(const quat_of<T>& A,
                                  const quat_of<T>& B) noexcept {
        return { A.v - B.v,
                 A.w - B.w };
    }

    template<typename T>
    LMATH_OUT quat_of<T> quat_scale(const quat_of<T>& Q, T s) noexcept {
        return { Q.v * s,
                 Q.w * s };
    }

    template<typename T>
    LMATH_OUT T quat_dot(const quat_of<T>& A,
                         const quat_of<T>& B) noexcept {
        return vec_dot(A.v, B.v) + A.w*B.w;
    }

    template<typename T>
    LMATH_OUT T quat_len(const quat_of<T>& Q) noexcept {
        return ::lm::sqrtf(quat_dot(Q, Q));
    }

    template<typename T>
    LMATH_OUT quat_of<T> quat_norm(const quat_of<T>& Q) noexcept {
        const T LEN = quat_len(Q);
        return LEN==T(0) ? quat_of<T>{} : quat_scale(Q, T(1)/LEN);
    }

    // ============================================================
    // Conjugate
    // ============================================================

    template<typename T>
    LMATH_OUT quat_of<T> quat_conj(const quat_of<T>& Q) noexcept {
        return { -Q.v, Q.w };
    }

    // ============================================================
    // Quaternion multiplication
    // ============================================================

    template<typename T>
    LMATH_OUT quat_of<T> quat_mul(const quat_of<T>& P,
                                  const quat_of<T>& Q) noexcept {
        return {
            vec3_cross(P.v,Q.v) + (P.v * Q.w) + (Q.v * P.w),
            (P.w * Q.w) - vec_dot(P.v,Q.v)
        };
    }

    // ============================================================
    // From axis-angle
    // ============================================================

    template<typename T>
    LMATH_OUT quat_of<T> quat_rotate(              T angle,
                                     const vec<T,3>& axis) noexcept {
        vec<T,3> n = vec_norm(axis);
        T s = ::lm::sinf(angle * T(0.5));
        T c = ::lm::cosf(angle * T(0.5));
        return { n * s, c };
    }

    // ============================================================
    // Rotate vector
    // ============================================================

    template<typename T>
    LMATH_OUT vec<T,3> quat_mul_vec3(const quat_of<T>& Q,
                                       const vec<T,3>& V) noexcept {
        // ryg's method
        const vec<T,3> C = vec3_cross(Q.v,V) * T(2);
        return V + C*Q.w + vec3_cross(Q.v,C);
    }

    // ============================================================
    // Quaternion <-> mat4
    // ============================================================

    template<typename T>
    LMATH_OUT mat4_of<T> mat4_from_quat(const quat_of<T>& Q) noexcept {
        const T A = Q.w;
        const T B = Q.v[0];
        const T C = Q.v[1];
        const T D = Q.v[2];

        const T A2=A*A, B2=B*B, C2=C*C, D2=D*D;
        mat4_of<T> M{};

        M[0][0] = A2+B2-C2-D2;
        M[0][1] = T(2) * (B*C + A*D);
        M[0][2] = T(2) * (B*D - A*C);

        M[1][0] = T(2) * (B*C - A*D);
        M[1][1] = A2-B2+C2-D2;
        M[1][2] = T(2) * (C*D + A*B);

        M[2][0] = T(2) * (B*D + A*C);
        M[2][1] = T(2) * (C*D - A*B);
        M[2][2] = A2-B2-C2+D2;

        M[3][3] = T(1);
        return M;
    }

    template<typename T>
    LMATH_OUT quat_of<T> quat_from_mat4(const mat4_of<T>& M) noexcept {
        T r{};
        int p[3]{0,1,2};

        for (int i = 0; i < 3; ++i) {
            if (M[i][i] > r) {
                r = M[i][i];
                p[0] = i;
                p[1] = (i+1)%3;
                p[2] = (i+2)%3;
            }
        }

        r = ::lm::sqrtf(
            T(1) + M[p[0]][p[0]] - M[p[1]][p[1]] - M[p[2]][p[2]]
        );

        if (r < T(1e-6))
            return { {T(1),T(0),T(0)}, T(0) };

        T inv = T(1) / (T(2) * r);

        return {
            {
                r * T(0.5),
                (M[p[0]][p[1]] - M[p[1]][p[0]]) * inv,
                (M[p[2]][p[0]] - M[p[0]][p[2]]) * inv
            },
            (M[p[2]][p[1]] - M[p[1]][p[2]]) * inv
        };
    }

    // ============================================================
    // Operators
    // ============================================================

    template<typename T>
    LMATH_OUT quat_of<T> operator+ (const quat_of<T>& A,
                                    const quat_of<T>& B) noexcept { return quat_add(A,B); }
    template<typename T>
    LMATH_OUT quat_of<T> operator- (const quat_of<T>& A,
                                    const quat_of<T>& B) noexcept { return quat_sub(A,B); }
    template<typename T>
    LMATH_OUT quat_of<T> operator* (const quat_of<T>& Q,
                                                    T s) noexcept { return quat_scale(Q,s); }
    template<typename T>
    LMATH_OUT quat_of<T> operator* (const quat_of<T>& A,
                                    const quat_of<T>& B) noexcept { return quat_mul(A,B); }
    template<typename T>
    LMATH_OUT bool operator== (const quat_of<T>& A,
                               const quat_of<T>& B) noexcept { return A.v==B.v && A.w==B.w; }
    template<typename T>
    LMATH_OUT bool operator!= (const quat_of<T>& A,
                               const quat_of<T>& B) noexcept { return !(A==B); }

} // namespace lm
