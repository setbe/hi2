#define LMATH_FORCE_NO_SIMD

// this port
#include "../linmath/vec.hpp"
#include "../linmath/mat.hpp"
#include "../linmath/quat.hpp"
extern "C" {
#   include "../3rd-party/linmath.h" // original copy
}
#include "../3rd-party/glm-1.0.3/glm/glm.hpp" // 3rd-party 'glm'
#include "../3rd-party/glm-1.0.3/glm/gtc/matrix_transform.hpp"

template<typename T>
inline void escape(const T& v) {
#ifdef _MSC_VER
    volatile const T* p = &v;
    (void)p;
#else
    asm volatile("" : : "g"(v) : "memory");
#endif
}

static volatile float dummy_float;

static lm::mat4 lm_dummy_mat4;
static lm::vec3 lm_dummy_vec3;
static lm::vec4 lm_dummy_vec4;

static ::mat4x4 c_dummy_mat4;
static ::vec3 c_dummy_vec3;
static ::vec4 c_dummy_vec4;

static glm::mat4 glm_dummy_mat4;
static glm::vec3 glm_dummy_vec3;
static glm::vec4 glm_dummy_vec4;




#include <chrono>
#include <cstdio>

using highres_clock = std::chrono::high_resolution_clock;


struct bench_result {
    const char* name;
    double ms;
};

template<typename Fn>
bench_result run_bench(const char* name, Fn&& fn, std::size_t iters) {
    // warm up
    const std::size_t warmup = iters / 5;
    for (std::size_t i = 0; i < warmup; ++i)
        fn();

    // actual bench
    auto t0 = highres_clock::now();
    for (std::size_t i = 0; i < iters; ++i)
        fn();
    auto t1 = highres_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return { name, ms };
}

bench_result bench_sqrtf_lm(std::size_t iters) {
    float x = 59.0f;
    float r{};
    return run_bench("lm::sqrtf", [&] {
        r = lm::sqrtf(x);
        dummy_float = r;
        escape(dummy_float);
        x += 0.000001f;
        if (x > 60.0f) x = 59.0f;
    }, iters);
}

bench_result bench_rsqrtf_lm(std::size_t iters) {
    float x = 59.0f;
    float r{};
    return run_bench("lm::rsqrtf", [&] {
        r = lm::rsqrtf(x);
        dummy_float = r;
        escape(dummy_float);
        x += 0.000001f;
        if (x > 60.0f) x = 59.0f;
    }, iters);
}

bench_result bench_vec3_dot_lm(std::size_t iters) {
    lm::vec3 a{ 1.f,2.f,3.f }, b{ 4.f,5.f,6.f };
    float r{};

    return run_bench("lm::vec3 dot", [&] {
        r = lm::vec_dot(a, b);
        dummy_float = r;
        escape(dummy_float);
    }, iters);
}

bench_result bench_vec3_dot_glm(std::size_t iters) {
    glm::vec3 a{ 1.f,2.f,3.f }, b{ 4.f,5.f,6.f };
    float r{};

    return run_bench("glm::vec3 dot", [&] {
        r = glm::dot(a, b);
        dummy_float = r;
        escape(dummy_float);
    }, iters);
}

bench_result bench_vec3_dot_c(std::size_t iters) {
    ::vec3 a{ 1.f,2.f,3.f }, b{ 4.f,5.f,6.f };
    float r{};

    return run_bench("linmath vec3 dot", [&] {
        r = ::vec3_mul_inner(a, b);
        dummy_float = r;
        escape(dummy_float);
    }, iters);
}

bench_result bench_vec3_norm_lm(std::size_t iters) {
    lm::vec3 v{ -1.f, 3.f, -7.f };
    lm::vec3 r{};
    return run_bench("lm::vec3 norm", [&] {
        r = lm::vec_norm(v);
        lm_dummy_vec3 = r;
        escape(lm_dummy_vec3);
    }, iters);
}

bench_result bench_vec3_norm_glm(std::size_t iters) {
    glm::vec3 v{ -1.f, 3.f, -7.f };
    glm::vec3 r{};
    return run_bench("glm::vec3 norm", [&] {
        r = glm::normalize(v);
        glm_dummy_vec3 = r;
        escape(glm_dummy_vec3);
    }, iters);
}

bench_result bench_vec3_norm_c(std::size_t iters) {
    ::vec3 v{ -1.f, 3.f, -7.f };
    ::vec3 r{};
    return run_bench("linmath vec3 norm", [&] {
        ::vec3_norm(r, v);
        ::vec3_dup(c_dummy_vec3, r);
        escape(c_dummy_vec3);
    }, iters);
}

bench_result bench_vec3_cross_lm(std::size_t iters) {
    lm::vec3 a{ 1.f, 2.f, 3.f };
    lm::vec3 b{ 4.f, 5.f, 6.f };
    lm::vec3 r{};
    return run_bench("lm::vec3 cross", [&] {
        r = lm::vec3_cross(a, b);
        lm_dummy_vec3 = r;
        escape(lm_dummy_vec3);

        a[0] += 0.000001f;
        if (a[0] > 2.f) a[0] = 1.f;
    }, iters);
}

bench_result bench_vec3_cross_glm(std::size_t iters) {
    glm::vec3 a{ 1.f, 2.f, 3.f };
    glm::vec3 b{ 4.f, 5.f, 6.f };
    glm::vec3 r{};
    return run_bench("glm::vec3 cross", [&] {
        r = glm::cross(a, b);
        glm_dummy_vec3 = r;
        escape(glm_dummy_vec3);

        a[0] += 0.000001f;
        if (a[0] > 2.f) a[0] = 1.f;
    }, iters);
}

bench_result bench_vec3_cross_c(std::size_t iters) {
    ::vec3 a{ 1.f, 2.f, 3.f };
    ::vec3 b{ 4.f, 5.f, 6.f };
    ::vec3 r{};
    return run_bench("linmath vec3 cross", [&] {
        ::vec3_mul_cross(r, a, b);
        ::vec3_dup(c_dummy_vec3, r);
        escape(c_dummy_vec3);

        a[0] += 0.000001f;
        if (a[0] > 2.f) a[0] = 1.f;
    }, iters);
}

bench_result bench_mat4_mul_lm(std::size_t iters) {
    lm::mat4 A = lm::mat4_rotate_x(0.7f);
    lm::mat4 B = lm::mat4_rotate_y(1.3f);
    lm::mat4 R{};

    return run_bench("lm::mat4 mul", [&] {
        R = lm::mat4_mul(A, B);
        lm_dummy_mat4 = R;
        escape(lm_dummy_mat4);
    }, iters);
}

bench_result bench_mat4_mul_glm(std::size_t iters) {
    glm::mat4 A = glm::rotate(glm::mat4(1.f), 0.7f, { 1,0,0 });
    glm::mat4 B = glm::rotate(glm::mat4(1.f), 1.3f, { 0,1,0 });
    glm::mat4 R{};

    return run_bench("glm::mat4 mul", [&] {
        R = A * B;
        glm_dummy_mat4 = R;
        escape(glm_dummy_mat4);
    }, iters);
}

bench_result bench_mat4_mul_c(std::size_t iters) {
    ::mat4x4 A{}, B{}, R{};
    ::mat4x4_identity(A);
    ::mat4x4_identity(B);

    ::mat4x4_rotate_X(A, A, 0.7f);
    ::mat4x4_rotate_Y(B, B, 1.3f);

    return run_bench("linmath mat4 mul", [&] {
        ::mat4x4_mul(R, A, B);
        ::mat4x4_dup(c_dummy_mat4, R);
        escape(c_dummy_mat4);
    }, iters);
}

bench_result bench_mat4_vec4_lm(std::size_t iters) {
    lm::mat4 M = lm::mat4_translate(1.f, 2.f, 3.f);
    lm::vec4 V{ 1.f,2.f,3.f,1.f };
    lm::vec4 R{};

    return run_bench("lm::mat4 * vec4", [&] {
        R = M * V;
        lm_dummy_vec4 = R;
        escape(lm_dummy_vec4);
    }, iters);
}

bench_result bench_mat4_vec4_glm(std::size_t iters) {
    glm::mat4 M = glm::translate(glm::mat4(1.f), { 1,2,3 });
    glm::vec4 V{ 1,2,3,1 };
    glm::vec4 R{};

    return run_bench("glm::mat4 * vec4", [&] {
        R = M * V;
        glm_dummy_vec4 = R;
        escape(glm_dummy_vec4);
    }, iters);
}

bench_result bench_mat4_vec4_c(std::size_t iters) {
    ::mat4x4 M{};
    ::vec4 V{ 1,2,3,1 };
    ::vec4 R{};
    ::mat4x4_translate(M, 1, 2, 3);

    return run_bench("linmath mat4 * vec4", [&] {
        ::mat4x4_mul_vec4(R, M, V);
        ::vec4_dup(c_dummy_vec4, R);
        escape(c_dummy_vec4);
    }, iters);
}

bench_result bench_mat4_translate_lm(std::size_t iters) {
    lm::mat4 R{};
    return run_bench("lm::mat4 translate", [&] {
        R = lm::mat4_translate(1.f, 2.f, 3.f);
        lm_dummy_mat4 = R;
        escape(lm_dummy_mat4);

        R[0][0] += 0.000001f;
        if (R[0][0] > 2.f) R[0][0] = 1.f;
    }, iters);
}

bench_result bench_mat4_translate_glm(std::size_t iters) {
    glm::mat4 R{};
    return run_bench("glm::mat4 translate", [&] {
        R = glm::translate(glm::mat4(1.f), glm::vec3(1.f, 2.f, 3.f));
        glm_dummy_mat4 = R;
        escape(glm_dummy_mat4);

        R[0][0] += 0.000001f;
        if (R[0][0] > 2.f) R[0][0] = 1.f;
    }, iters);
}

bench_result bench_mat4_translate_c(std::size_t iters) {
    ::mat4x4 R{};
    return run_bench("linmath mat4 translate", [&] {
        ::mat4x4_translate(R, 1.f, 2.f, 3.f);
        ::mat4x4_dup(c_dummy_mat4, R);
        escape(c_dummy_mat4);

        R[0][0] += 0.000001f;
        if (R[0][0] > 2.f) R[0][0] = 1.f;
    }, iters);
}

bench_result bench_mat4_rotate_x_lm(std::size_t iters) {
    lm::mat4 R{};
    return run_bench("lm::mat4 rotate_x", [&] {
        R = lm::mat4_rotate_x(0.7f);
        lm_dummy_mat4 = R;
        escape(lm_dummy_mat4);

        R[0][0] += 0.000001f;
        if (R[0][0] > 2.f) R[0][0] = 1.f;
    }, iters);
}

bench_result bench_mat4_rotate_x_glm(std::size_t iters) {
    glm::mat4 R{};
    return run_bench("glm::mat4 rotate_x", [&] {
        R = glm::rotate(glm::mat4(1.f), 0.7f, glm::vec3(1.f, 0.f, 0.f));
        glm_dummy_mat4 = R;
        escape(glm_dummy_mat4);

        R[0][0] += 0.000001f;
        if (R[0][0] > 2.f) R[0][0] = 1.f;
    }, iters);
}

bench_result bench_mat4_rotate_x_c(std::size_t iters) {
    ::mat4x4 I{}, R{};
    ::mat4x4_identity(I);
    return run_bench("linmath mat4 rotate_x", [&] {
        ::mat4x4_rotate_X(R, I, 0.7f);
        ::mat4x4_dup(c_dummy_mat4, R);
        escape(c_dummy_mat4);

        R[0][0] += 0.000001f;
        if (R[0][0] > 2.f) R[0][0] = 1.f;
    }, iters);
}

bench_result bench_mat4_look_at_lm(std::size_t iters) {
    lm::vec3 eye{ 1.5f, -2.0f, 4.0f };
    lm::vec3 center{ 0.5f, 1.0f, -3.0f };
    lm::vec3 up{ 0.0f, 1.0f, 0.0f };
    lm::mat4 R{};

    return run_bench("lm::mat4 look_at", [&] {
        R = lm::mat4_look_at(eye, center, up);
        lm_dummy_mat4 = R;
        escape(lm_dummy_mat4);
    }, iters);
}

bench_result bench_mat4_look_at_glm(std::size_t iters) {
    glm::vec3 eye{ 1.5f, -2.0f, 4.0f };
    glm::vec3 center{ 0.5f, 1.0f, -3.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    glm::mat4 R{};

    return run_bench("glm::mat4 lookAt", [&] {
        R = glm::lookAt(eye, center, up);
        glm_dummy_mat4 = R;
        escape(glm_dummy_mat4);
    }, iters);
}

bench_result bench_mat4_look_at_c(std::size_t iters) {
    ::vec3 eye{ 1.5f, -2.0f, 4.0f };
    ::vec3 center{ 0.5f, 1.0f, -3.0f };
    ::vec3 up{ 0.0f, 1.0f, 0.0f };
    ::mat4x4 R{};

    return run_bench("linmath mat4 look_at", [&] {
        ::mat4x4_look_at(R, eye, center, up);
        ::mat4x4_dup(c_dummy_mat4, R);
        escape(c_dummy_mat4);
    }, iters);
}

int main() {
    constexpr std::size_t iters = 200'000'000;

    bench_result results[] = {
        bench_sqrtf_lm(iters),
        bench_rsqrtf_lm(iters),

        bench_vec3_dot_lm(iters),
        bench_vec3_dot_glm(iters),
        bench_vec3_dot_c(iters),

        bench_vec3_norm_lm(iters),
        bench_vec3_norm_glm(iters),
        bench_vec3_norm_c(iters),

        bench_vec3_cross_lm(iters),
        bench_vec3_cross_glm(iters),
        bench_vec3_cross_c(iters),

        bench_mat4_translate_lm(iters),
        bench_mat4_translate_glm(iters),
        bench_mat4_translate_c(iters),

        bench_mat4_rotate_x_lm(iters),
        bench_mat4_rotate_x_glm(iters),
        bench_mat4_rotate_x_c(iters),

        bench_mat4_mul_lm(iters),
        bench_mat4_mul_glm(iters),
        bench_mat4_mul_c(iters),

        bench_mat4_vec4_lm(iters),
        bench_mat4_vec4_glm(iters),
        bench_mat4_vec4_c(iters),

        bench_mat4_look_at_lm(iters),
        bench_mat4_look_at_glm(iters),
        bench_mat4_look_at_c(iters),
    };

    for (auto& r : results)
        std::printf("%-24s : %8.2f ms\n", r.name, r.ms);

    std::printf("dummy lm::mat4 %8.2f\n", lm_dummy_mat4[0][0]);
    std::printf("dummy lm::vec4 %8.2f\n", lm_dummy_vec4[0]);
    std::printf("dummy lm::vec3 %8.2f\n", lm_dummy_vec3[0]);
    std::printf("dummy linmath mat4 %8.2f\n", c_dummy_mat4[0][0]);
    std::printf("dummy linmath vec4 %8.2f\n", c_dummy_vec4[0]);
    std::printf("dummy linmath vec3 %8.2f\n", c_dummy_vec3[0]);
    std::printf("dummy glm::mat4 %8.2f\n", glm_dummy_mat4[0][0]);
    std::printf("dummy glm::vec4 %8.2f\n", glm_dummy_vec4[0]);
    std::printf("dummy glm::vec3 %8.2f\n", glm_dummy_vec3[0]);
}
