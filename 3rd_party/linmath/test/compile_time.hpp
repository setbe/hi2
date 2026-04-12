#pragma once
#include "../linmath/vec.hpp"
#include "../linmath/mat.hpp"
#include "../linmath/quat.hpp"


// Enable C++17 or higher for compile-time tests
#ifdef LMATH_CXX17
#include <type_traits>
static_assert(std::is_trivially_copyable_v<lm::vec3>);
static_assert(std::is_standard_layout_v<lm::vec3>);

static_assert(std::is_trivially_copyable_v<lm::mat4>);
static_assert(std::is_standard_layout_v<lm::mat4>);

namespace lm {
    namespace ct { // compile-time
        // ------------------------------------------------------------
        // vec constexpr arithmetic validation
        // ------------------------------------------------------------
        LMATH_CONSTEVAL bool test_vec_arithmetic() noexcept {
            using V = lm::vec3;

            constexpr V a{ 1.f, 2.f, 3.f };
            constexpr V b{ 4.f, 5.f, 6.f };

            // + -
            constexpr V add = a+b;
            static_assert(add[0]==5.f &&
                          add[1]==7.f &&
                          add[2]==9.f);

            constexpr V sub = b-a;
            static_assert(sub[0]==3.f &&
                          sub[1]==3.f &&
                          sub[2]==3.f);

            // unary -
            constexpr V neg = -a;
            static_assert(neg[0]==-1.f &&
                          neg[1]==-2.f &&
                          neg[2]==-3.f);

            // scalar *
            constexpr V mul1 = a*2.f;
            constexpr V mul2 = 2.f*a;
            static_assert(mul1[0]==2.f &&
                          mul2[2]==6.f);

            // scalar /
            constexpr V div = a / 2.f;
            static_assert(div[0]==0.5f &&
                          div[2]==1.5f);

            // dot
            constexpr float d = lm::vec_dot(a,b);
            static_assert(d==32.f);

            // length
            constexpr float len2 = lm::vec_dot(a,a);
            static_assert(len2==14.f);

            // comparisons
            static_assert(a==a);
            static_assert(a!=b);

            return true;
        } // test_vec_arithmetic


    // =============================================================
    // constexpr matrix arithmetic validation
    // =============================================================
        LMATH_CONSTEVAL bool test_mat_arithmetic() noexcept {

            using M3 = lm::mat3;
            using M4 = lm::mat4;
            using V3 = lm::vec3;
            using V4 = lm::vec4;

            // ---------------------------------------------------------
            // identity
            // ---------------------------------------------------------
            constexpr M3 I3 = lm::mat_identity<float, 3>();
            static_assert(I3[0][0]==1.f &&
                          I3[1][1]==1.f &&
                          I3[2][2]==1.f);
            static_assert(I3[0][1]==0.f &&
                          I3[2][1]==0.f);

            constexpr M4 I4 = lm::mat_identity<float, 4>();
            static_assert(I4[0][0]==1.f &&
                          I4[3][3]==1.f);
            static_assert(I4[1][3]==0.f);

            // ---------------------------------------------------------
            // add / sub
            // ---------------------------------------------------------
            constexpr M3 Z3{};               // zero matrix
            constexpr M3 S3 = I3+Z3;
            static_assert(S3[1][1]==1.f);

            constexpr M3 D3 = S3-I3;
            static_assert(D3[0][0]==0.f &&
                          D3[2][2]==0.f);

            // ---------------------------------------------------------
            // scalar mul
            // ---------------------------------------------------------
            constexpr M3 K3 = I3 * 2.f;
            static_assert(K3[0][0]==2.f);
            static_assert(K3[1][1]==2.f);

            constexpr M3 K3b = 0.5f * K3;
            static_assert(K3b[2][2]==1.f);

            // ---------------------------------------------------------
            // mat * mat
            // ---------------------------------------------------------
            constexpr M3 M3m = I3*I3;
            static_assert(M3m[0][0]==1.f &&
                          M3m[1][1]==1.f);

            // Compile-time test is ambiguous due to SIMD-spec
            // constexpr M4 M4m = I4*I4;
            // static_assert(M4m[2][2]==1.f &&
            //               M4m[3][3]==1.f);

            // ---------------------------------------------------------
            // mat * vec
            // ---------------------------------------------------------
            constexpr V3 v3{ 1.f, 2.f, 3.f };
            constexpr V3 r3 = I3*v3;
            static_assert(r3[0]==1.f && 
                          r3[2]==3.f);

            // Compile-time test is ambiguous due to SIMD-spec
            // constexpr V4 v4{ 1.f, 2.f, 3.f, 1.f };
            // constexpr V4 r4 = I4*v4;
            // static_assert(r4[1]==2.f &&
            //               r4[3]==1.f);

            // ---------------------------------------------------------
            // comparisons
            // ---------------------------------------------------------
            static_assert(I3==I3);
            static_assert(I4!=M4{});

            return true;
        } // test_mat_arithmetic



        template<typename A, typename B>
        LMATH_CONSTEVAL bool byte_equal(const A& a, const B& b) noexcept {
            static_assert(sizeof(A) == sizeof(B));
            const std::byte* pa = reinterpret_cast<const std::byte*>(&a);
            const std::byte* pb = reinterpret_cast<const std::byte*>(&b);

            for (int i=0; i<sizeof(A); ++i)
                if (pa[i] != pb[i]) return false;
            return true;
        }

        LMATH_CONSTEVAL bool feq(float a, float b, float eps = 1e-5f) noexcept {
            return (a>b ? a-b : b-a) <= eps;
        }

        LMATH_CONSTEVAL bool test_quat_arithmetic() noexcept {
            // --------------------------------------------
            // identity
            // --------------------------------------------
            quat q_id{ quat_identity() };

            if (!feq(q_id[0], 0.f)) return false;
            if (!feq(q_id[1], 0.f)) return false;
            if (!feq(q_id[2], 0.f)) return false;
            if (!feq(q_id[3], 1.f)) return false;

            // --------------------------------------------
            // multiplication with identity
            // --------------------------------------------
            quat q{ 1.f, 2.f, 3.f, 4.f };

            quat r1{ quat_mul(q, q_id) },
                 r2{ quat_mul(q_id, q) };

            for (int i = 0; i < 4; ++i) {
                if (!feq(r1[i], q[i])) return false;
                if (!feq(r2[i], q[i])) return false;
            }

            // --------------------------------------------
            // conjugate
            // --------------------------------------------
            quat qc{ quat_conj(q) };

            if (!feq(qc[0], -q[0])) return false;
            if (!feq(qc[1], -q[1])) return false;
            if (!feq(qc[2], -q[2])) return false;
            if (!feq(qc[3], q[3])) return false;

            // --------------------------------------------
            // q * conj(q) = (0,0,0, |q|^2)
            // --------------------------------------------
            quat qq{ quat_mul(q, qc) };

            if (!feq(qq[0], 0.f)) return false;
            if (!feq(qq[1], 0.f)) return false;
            if (!feq(qq[2], 0.f)) return false;

            const float len2 =
                q[0]*q[0] + q[1]*q[1] + q[2] * q[2] + q[3] * q[3];

            if (!feq(qq[3], len2)) return false;

            // --------------------------------------------
            // quat * vec3
            // identity must not rotate
            // --------------------------------------------
            vec3 v{ 1.f, 2.f, 3.f };
            vec3 vr{ quat_mul_vec3(q_id, v) };

            for (int i=0; i<3; ++i)
                if (!feq(vr[i], v[i])) return false;

            return true;
}
    } // namespace ct (compile-time)
} // namespace lm
#endif // LMATH_CXX17