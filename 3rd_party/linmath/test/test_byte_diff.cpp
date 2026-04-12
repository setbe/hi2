#define CATCH_CONFIG_MAIN
#include "../3rd-party/catch.hpp"

// this port
#include "../linmath/vec.hpp"
#include "../linmath/mat.hpp"
#include "../linmath/quat.hpp"

extern "C" {
#   include "../3rd-party/linmath.h" // original copy
}

#include "../3rd-party/glm-1.0.3/glm/glm.hpp" // 3rd-party 'glm' also
#include "../3rd-party/glm-1.0.3/glm/gtc/matrix_transform.hpp"

// Compile-time tests for C++17 version or higher
#ifdef LMATH_CXX17
#   include "compile_time.hpp"

    // Guarantee compile-time control
    static_assert(lm::ct::test_vec_arithmetic(), "constexpr vec arithmetic failed");
    static_assert(lm::ct::test_mat_arithmetic(), "constexpr mat arithmetic failed");
    static_assert(lm::ct::test_quat_arithmetic(), "constexpr quat arithmetic failed");
#endif

namespace {
    template<typename A, typename B>
    void dump_mat4_diff(const A& a, const B& b) {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                std::printf("[%d][%d]  a=% .9f  b=% .9f  d=% .9f\n",
                    c, r, a[c][r], b[c][r], a[c][r] - b[c][r]);
            }
        }
        std::printf("---------------\n");
    }

    template<typename A, typename B>
    bool byte_equal(const A& a, const B& b) {
        static_assert(sizeof(A) == sizeof(B));
        return std::memcmp(&a, &b, sizeof(A)) == 0;
    }

    template<typename A, typename B>
    bool mat4_approx_equal(const A& a, const B& b, float eps = 1e-5f) {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                if (std::fabs(a[c][r] - b[c][r]) > eps)
                    return false;
        return true;
    }

    TEST_CASE("sqrtf sanity", "[math]") {
        REQUIRE(::lm::sqrtf(59.f) == Approx(7.6811457f).margin(1e-6f));
    }

    TEST_CASE("vec3 normalize sanity", "[vec3][math]") {
        lm::vec3 v{ -1.f, 3.f, -7.f };
        auto n = lm::vec_norm(v);

        REQUIRE(n[0] == Approx(-1.f / std::sqrt(59.f)).margin(1e-6f));
        REQUIRE(n[1] == Approx(3.f / std::sqrt(59.f)).margin(1e-6f));
        REQUIRE(n[2] == Approx(-7.f / std::sqrt(59.f)).margin(1e-6f));
    }

    TEST_CASE("vec3 arithmetic matches linmath.h", "[vec3]") {
        // lm
        lm::vec3 a{ 1.f, 2.f, 3.f };
        lm::vec3 b{ 4.f, 5.f, 6.f };

        lm::vec3 cpp_add = a + b;
        lm::vec3 cpp_sub = a - b;
        float    cpp_dot = lm::vec_dot(a, b);

        // glm
        glm::vec3 ga{ 1.f, 2.f, 3.f };
        glm::vec3 gb{ 4.f, 5.f, 6.f };

        glm::vec3 g_add = ga + gb;
        glm::vec3 g_sub = ga - gb;
        float     g_dot = glm::dot(ga, gb);

        // C
        ::vec3 ca{ 1.f, 2.f, 3.f };
        ::vec3 cb{ 4.f, 5.f, 6.f };
        ::vec3 c_add{}, c_sub{};
        ::vec3_add(c_add, ca, cb);
        ::vec3_sub(c_sub, ca, cb);
        float c_dot = ::vec3_mul_inner(ca, cb);

        // lm/C
        REQUIRE(byte_equal(cpp_add, c_add));
        REQUIRE(byte_equal(cpp_sub, c_sub));
        REQUIRE(cpp_dot == Approx(c_dot));

        // lm/glm
        REQUIRE(byte_equal(cpp_add, g_add));
        REQUIRE(byte_equal(cpp_sub, g_sub));
        REQUIRE(cpp_dot == Approx(g_dot));

        // glm/C
        REQUIRE(byte_equal(g_add, c_add));
        REQUIRE(byte_equal(g_sub, c_sub));
        REQUIRE(Approx(g_dot) == c_dot);
    }

    TEST_CASE("vec3 cross & reflect matches linmath.h", "[vec3]") {
        // lm
        lm::vec3 v{ 1.f, 2.f, 3.f };
        lm::vec3 n{ 0.f, 1.f, 0.f };
        auto cpp_cross = lm::vec3_cross(v, n);
        auto cpp_ref = lm::vec3_reflect(v, n);

        // glm
        glm::vec3 gv{ 1.f, 2.f, 3.f };
        glm::vec3 gn{ 0.f, 1.f, 0.f };
        auto g_cross = glm::cross(gv, gn);
        auto g_ref = glm::reflect(gv, gn);

        // C
        ::vec3 cv{ 1.f, 2.f, 3.f };
        ::vec3 cn{ 0.f, 1.f, 0.f };
        ::vec3 c_cross{}, c_ref{};
        ::vec3_mul_cross(c_cross, cv, cn);
        ::vec3_reflect(c_ref, cv, cn);

        // lm/C
        REQUIRE(byte_equal(cpp_cross, c_cross));
        REQUIRE(byte_equal(cpp_ref, c_ref));
        // lm/glm
        REQUIRE(byte_equal(cpp_cross, g_cross));
        REQUIRE(byte_equal(cpp_ref, g_ref));
        // glm/C
        REQUIRE(byte_equal(g_cross, c_cross));
        REQUIRE(byte_equal(g_ref, c_ref));
    }

    TEST_CASE("mat4 identity / translate matches glm & linmath.h", "[mat4][glm]") {
        // lm
        lm::mat4 cpp_I = lm::mat_identity<float, 4>();
        lm::mat4 cpp_T = lm::mat4_translate(1.f, 2.f, 3.f);

        // glm
        glm::mat4 g_I(1.f);
        glm::mat4 g_T = glm::translate(glm::mat4(1.f),
            glm::vec3(1.f, 2.f, 3.f));

        // C
        ::mat4x4 c_I{}, c_T{};
        ::mat4x4_identity(c_I);
        ::mat4x4_translate(c_T, 1.f, 2.f, 3.f);
 
        // lm/C
        REQUIRE(byte_equal(cpp_I, c_I));
        REQUIRE(byte_equal(cpp_T, c_T));
        // lm/glm
        REQUIRE(byte_equal(cpp_I, g_I));
        REQUIRE(byte_equal(cpp_T, g_T));
        // glm/C
        REQUIRE(byte_equal(g_I, c_I));
        REQUIRE(byte_equal(g_T, c_T));
    }


    TEST_CASE("mat4 multiply matches glm & linmath.h", "[mat4][glm]") {
        // lm
        lm::mat4 A = lm::mat4_translate(1.f, 2.f, 3.f);
        lm::mat4 B = lm::mat4_scale(2.f, 3.f, 4.f);
        lm::mat4 R = lm::mat4_mul(A,B);

        // glm
        glm::mat4 gA = glm::translate(glm::mat4(1.f), glm::vec3(1.f, 2.f, 3.f));
        glm::mat4 gB = glm::scale(glm::mat4(1.f), glm::vec3(2.f, 3.f, 4.f));
        glm::mat4 gR = gA * gB;

        // C
        ::mat4x4 cA{}, cB{}, cR{};
        ::mat4x4_translate(cA, 1.f, 2.f, 3.f);
        ::mat4x4_identity(cB);
        ::mat4x4_scale_aniso(cB, cB, 2.f, 3.f, 4.f);
        ::mat4x4_mul(cR, cA, cB);
   
        REQUIRE(byte_equal(R, cR));  // lm  / C
        REQUIRE(byte_equal(R, gR));  // lm  / glm
        REQUIRE(byte_equal(gR, cR)); // glm / C
    }


    TEST_CASE("mat4 * vec4 matches glm & linmath.h", "[mat4][vec4][glm]") {
        // lm
        lm::mat4 M = lm::mat4_translate(1.f, 2.f, 3.f);
        lm::vec4 V{ 1.f, 2.f, 3.f, 1.f };
        lm::vec4 R = M * V;

        // glm
        glm::mat4 gM = glm::translate(glm::mat4(1.f), glm::vec3(1.f, 2.f, 3.f));
        glm::vec4 gV{ 1.f, 2.f, 3.f, 1.f };
        glm::vec4 gR = gM * gV;

        // C
        ::mat4x4 cM{};
        ::vec4   cV{ 1.f, 2.f, 3.f, 1.f };
        ::vec4   cR{};
        ::mat4x4_translate(cM, 1.f, 2.f, 3.f);
        ::mat4x4_mul_vec4(cR, cM, cV);

        REQUIRE(byte_equal(R, cR));
        REQUIRE(byte_equal(R, gR));
        REQUIRE(byte_equal(gR, cR));
    }

    TEST_CASE("mat4 SIMD path equals scalar path", "[mat4][simd]") {
        lm::mat4 A = lm::mat4_rotate_x(0.7f);
        lm::mat4 B = lm::mat4_rotate_y(1.3f);

        lm::mat4 R1 = lm::mat4_mul(A, B);
        lm::mat4 R2 = lm::mat4_mul_scalar(A, B);

        REQUIRE(byte_equal(R1, R2));
    }



    TEST_CASE("mat3 basic operations", "[mat3]") {
        lm::mat3 m{ lm::mat_identity<float, 3>() };
        for (int i=0; i<3; ++i)
            for (int j=0; j<3; ++j)
                REQUIRE(m[i][j] == Approx(i==j ? 1.f : 0.f));
    }

    
    TEST_CASE("quat arithmetic", "[quat]") {

        lm::quat q1{ 1.f, 2.f, 3.f, 4.f },
                 q2{ 5.f, 6.f, 7.f, 8.f };

        ::quat cq1{ 1.f, 2.f, 3.f, 4.f },
               cq2{ 5.f, 6.f, 7.f, 8.f };

        lm::quat r{ lm::quat_identity() };
        ::quat cr;
        ::quat_identity(cr);

        SECTION("identity") {
            REQUIRE(std::memcmp(&r, cr, sizeof(lm::quat))==0);
        }

        SECTION("mul") {
            r = lm::quat_mul(q1, q2);

            ::quat_mul(cr, cq1, cq2);

            REQUIRE(std::memcmp(&r, cr, sizeof(lm::quat))==0);
        }

        SECTION("conjugate") {
            r = lm::quat_conj(q1);

            ::quat_conj(cr, cq1);

            REQUIRE(std::memcmp(&r, cr, sizeof(lm::quat))==0);
        }

        SECTION("mul_vec3") {
            lm::vec3 v{ 1.f, 0.5f, -2.f };
            ::vec3  cv{ 1.f, 0.5f, -2.f };

            lm::vec3 r2{ lm::quat_mul_vec3(q1, v) };

            ::vec3 cr2{};
            ::quat_mul_vec3(cr2, cq1, cv);

            REQUIRE(std::memcmp(&r2, cr2, sizeof(vec3))==0);
        }
    }


    TEST_CASE("mat4 look_at matches glm & linmath.h", "[mat4][look_at][glm]") {
        // test data
        lm::vec3 eye{ 1.5f, -2.0f,  4.0f };
        lm::vec3 center{ 0.5f,  1.0f, -3.0f };
        lm::vec3 up{ 0.0f,  1.0f,  0.0f };

        // lm
        lm::mat4 cpp_M = lm::mat4_look_at(eye, center, up);

        // glm
        glm::mat4 g_M = glm::lookAt(
            glm::vec3(eye[0], eye[1], eye[2]),
            glm::vec3(center[0], center[1], center[2]),
            glm::vec3(up[0], up[1], up[2])
        );

        // C
        ::mat4x4 c_M{};
        ::vec3 c_eye{    eye[0],    eye[1],    eye[2] };
        ::vec3 c_center{ center[0], center[1], center[2] };
        ::vec3 c_up{     up[0],     up[1],     up[2] };
        ::mat4x4_look_at(c_M, c_eye, c_center, c_up);

        // lm / C
        dump_mat4_diff(cpp_M, c_M);
        REQUIRE(mat4_approx_equal(cpp_M, c_M, 2e-5f));

        // lm / glm
        dump_mat4_diff(cpp_M, g_M);
        REQUIRE(mat4_approx_equal(cpp_M, g_M, 2e-5f));

        // glm / C
        dump_mat4_diff(g_M, c_M);
        REQUIRE(mat4_approx_equal(g_M, c_M, 2e-5f));

        // extra functional check: transform eye -> origin
        lm::vec4 eye4{ eye[0], eye[1], eye[2], 1.f };
        lm::vec4 cpp_eye_view = cpp_M * eye4;

        glm::vec4 g_eye4{ eye[0], eye[1], eye[2], 1.f };
        glm::vec4 g_eye_view = g_M * g_eye4;

        REQUIRE(cpp_eye_view[0] == Approx(0.f).margin(1e-5f));
        REQUIRE(cpp_eye_view[1] == Approx(0.f).margin(1e-5f));
        REQUIRE(cpp_eye_view[2] == Approx(0.f).margin(1e-5f));
        REQUIRE(cpp_eye_view[3] == Approx(1.f).margin(1e-5f));

        REQUIRE(g_eye_view[0] == Approx(0.f).margin(1e-5f));
        REQUIRE(g_eye_view[1] == Approx(0.f).margin(1e-5f));
        REQUIRE(g_eye_view[2] == Approx(0.f).margin(1e-5f));
        REQUIRE(g_eye_view[3] == Approx(1.f).margin(1e-5f));
    }
}