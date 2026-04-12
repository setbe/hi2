#define GLM_FORCE_SIMD

#include "../linmath/vec.hpp"
#include "../linmath/mat.hpp"
#include "../linmath/quat.hpp"

#include "../3rd-party/glm-1.0.3/glm/glm.hpp"
#include "../3rd-party/glm-1.0.3/glm/gtc/matrix_transform.hpp"

#include <chrono>
#include <cstdio>

using highres_clock = std::chrono::high_resolution_clock;

// ---------------- escape ----------------
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
static lm::vec4 lm_dummy_vec4;

static lm::vec3 lm_dummy_vec3;
static glm::vec3 glm_dummy_vec3;

static glm::mat4 glm_dummy_mat4;
static glm::vec4 glm_dummy_vec4;

// ---------------- bench ----------------
struct bench_result {
    const char* name;
    double ms;
};

template<typename Fn>
bench_result run_bench(const char* name, Fn&& fn, std::size_t iters) {
    const std::size_t warmup = iters / 5;

    for (std::size_t i = 0; i < warmup; ++i) fn();

    auto t0 = highres_clock::now();
    for (std::size_t i = 0; i < iters; ++i) fn();
    auto t1 = highres_clock::now();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return { name, ms };
}


bench_result bench_sqrtf_lm(std::size_t iters) {
    float x = 59.0f;
    float r{};
    return run_bench("lm::sqrtf SIMD", [&] {
        r = lm::sqrtf(x);
        dummy_float = r;
        escape(r);
        x += 0.000001f;
        if (x > 60.0f) x = 59.0f;
    }, iters);
}

bench_result bench_rsqrtf_lm(std::size_t iters) {
    float x = 59.0f;
    float r{};
    return run_bench("lm::rsqrtf SIMD", [&] {
        r = lm::rsqrtf(x);
        dummy_float = r;
        escape(r);
        x += 0.000001f;
        if (x > 60.0f) x = 59.0f;
    }, iters);
}


// ---------------- vec3 dot ----------------
bench_result bench_vec3_dot_lm(std::size_t iters) {
    lm::vec3 a{ 1.f,2.f,3.f }, b{ 4.f,5.f,6.f };
    float r{};
    return run_bench("lm::vec3 dot SIMD", [&] {
        r = lm::vec_dot(a, b);
        dummy_float = r;
        escape(r);
    }, iters);
}

bench_result bench_vec3_dot_glm(std::size_t iters) {
    glm::vec3 a{ 1.f,2.f,3.f }, b{ 4.f,5.f,6.f };
    float r{};
    return run_bench("glm::vec3 dot SIMD", [&] {
        r = glm::dot(a, b);
        escape(r);
        dummy_float = r;
    }, iters);
}

// ---------------- mat4 mul ----------------
bench_result bench_mat4_mul_lm(std::size_t iters) {
    lm::mat4 A = lm::mat4_rotate_x(0.7f);
    lm::mat4 B = lm::mat4_rotate_y(1.3f);
    lm::mat4 R{};
    return run_bench("lm::mat4 mul SIMD", [&] {
        R = lm::mat4_mul(A, B);
        escape(R);
        lm_dummy_mat4 = R;
    }, iters);
}

bench_result bench_mat4_mul_glm(std::size_t iters) {
    glm::mat4 A = glm::rotate(glm::mat4(1.f), 0.7f, glm::vec3(1, 0, 0));
    glm::mat4 B = glm::rotate(glm::mat4(1.f), 1.3f, glm::vec3(0, 1, 0));
    glm::mat4 R{};
    return run_bench("glm::mat4 mul SIMD", [&] {
        R = A * B;
        escape(R);
        glm_dummy_mat4 = R;
    }, iters);
}

// ---------------- mat4 * vec4 ----------------
bench_result bench_mat4_vec4_lm(std::size_t iters) {
    lm::mat4 M = lm::mat4_translate(1.f, 2.f, 3.f);
    lm::vec4 V{ 1.f,2.f,3.f,1.f };
    lm::vec4 R{};
    return run_bench("lm::mat4 * vec4 SIMD", [&] {
        R = M * V;
        escape(R);
        lm_dummy_vec4 = R;
    }, iters);
}

bench_result bench_mat4_vec4_glm(std::size_t iters) {
    glm::mat4 M = glm::translate(glm::mat4(1.f), glm::vec3(1, 2, 3));
    glm::vec4 V{ 1.f,2.f,3.f,1.f };
    glm::vec4 R{};
    return run_bench("glm::mat4 * vec4 SIMD", [&] {
        R = M * V;
        escape(R);
        glm_dummy_vec4 = R;
    }, iters);
}

bench_result bench_mat4_look_at_lm(std::size_t iters) {
    lm::vec3 eye{ 1.5f, -2.0f, 4.0f };
    lm::vec3 center{ 0.5f, 1.0f, -3.0f };
    lm::vec3 up{ 0.0f, 1.0f, 0.0f };
    lm::mat4 R{};
    return run_bench("lm::mat4 look_at SIMD", [&] {
        R = lm::mat4_look_at(eye, center, up);
        escape(R);
        lm_dummy_mat4 = R;
    }, iters);
}

bench_result bench_mat4_look_at_glm(std::size_t iters) {
    glm::vec3 eye{ 1.5f, -2.0f, 4.0f };
    glm::vec3 center{ 0.5f, 1.0f, -3.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    glm::mat4 R{};
    return run_bench("glm::mat4 lookAt SIMD", [&] {
        R = glm::lookAt(eye, center, up);
        escape(R);
        glm_dummy_mat4 = R;
    }, iters);
}

// ---------------- main ----------------
int main() {
    std::printf("Current SIMD for `lm::` is: %s\n", lm::simd::level_string(lm::simd::max_level()));

    constexpr std::size_t iters = 200'000'000;

    bench_result results[] = {
        bench_sqrtf_lm(iters),
        bench_rsqrtf_lm(iters),

        bench_vec3_dot_lm(iters),
        bench_vec3_dot_glm(iters),
        
        bench_mat4_mul_lm(iters),
        bench_mat4_mul_glm(iters),
        
        bench_mat4_vec4_lm(iters),
        bench_mat4_vec4_glm(iters),

        bench_mat4_look_at_lm(iters),
        bench_mat4_look_at_glm(iters),
    };

    for (auto& r : results)
        std::printf("%-24s : %8.2f ms\n", r.name, r.ms);

    std::printf("dummy lm::mat4 %8.2f\n", lm_dummy_mat4[0][0]);
    std::printf("dummy lm::vec4 %8.2f\n", lm_dummy_vec4[0]);
    std::printf("dummy lm::vec3 %8.2f\n", lm_dummy_vec3[0]);
    
    std::printf("dummy glm::mat4 %8.2f\n", glm_dummy_mat4[0][0]);
    std::printf("dummy glm::vec4 %8.2f\n", glm_dummy_vec4[0]);
    std::printf("dummy glm::vec3 %8.2f\n", glm_dummy_vec3[0]);
}
