
# linmath.hpp

linmath.hpp is a freestanding, header-only linear algebra library for C++ designed with a strong emphasis on:

- predictable performance
- minimal dependencies
- SIMD acceleration
- freestanding compatibility

The project started as a modern C++ reinterpretation of linmath.h, aiming to preserve its simplicity and transparency while introducing:

- C++ ergonomics
- runtime SIMD dispatch
- optimized matrix/vector operations

The goal is a small, portable math library suitable for engines, tools, and freestanding environments.

---

# Key Features

- Freestanding friendly
- Header-only
- Runtime SIMD dispatch
- SSE2 / AVX / AVX2 / NEON support
- Scalar fallback
- No dynamic allocation
- No exceptions
- No RTTI
- Deterministic instruction cost
- Predictable memory layout

---

# SIMD Support

SIMD acceleration is optional and detected at runtime.

Supported execution paths:

| Mode | Architecture |
|-----|-----|
| Scalar | baseline |
| SSE2 | x86 |
| AVX / AVX2 | x86 |
| NEON | ARM |


SIMD can be disabled at compile time (define once *before* any include):

```cpp
#define LMATH_FORCE_NO_SIMD
```

---

# Benchmark Methodology

All benchmarks were executed with:

- 200,000,000 iterations
- 40,000,000 warm‑up iterations (not included in measured time)

Environment:

CPU: Intel Core i5‑9300H  
OS: Windows 10  
Compiler: MSVC

---

# Benchmark Results

## x64 — AVX2

| Function | Time |
|---|---|
| `lm`::`sqrtf` SIMD | 896.94 ms |
| `lm`::`rsqrtf` SIMD | 637.34 ms |
| `lm`::`vec3 dot` SIMD | 51.87 ms |
| `glm`::`vec3 dot` SIMD | 50.41 ms |
| `lm`::`mat4 mul` SIMD | 1389.33 ms |
| `glm`::`mat4 mul` SIMD | 4107.18 ms |
| `lm`::`mat4 * vec4` SIMD | 627.09 ms |
| `glm`::`mat4 * vec4` SIMD | 834.72 ms |
| `lm`::`mat4 look_at` SIMD | 5142.53 ms |
| `glm`::`mat4 lookAt` SIMD | 3739.55 ms |

## x64 — SSE2

| Function | Time |
|---|---|
| `lm`::`sqrtf` SIMD | 697.68 ms |
| `lm`::`rsqrtf` SIMD | 465.08 ms |
| `lm`::`vec3 dot` SIMD | 52.39 ms |
| `glm`::`vec3 dot` SIMD | 50.56 ms |
| `lm`::`mat4 mul` SIMD | 3644.14 ms |
| `glm`::`mat4 mul` SIMD | 4573.58 ms |
| `lm`::`mat4 * vec4` SIMD | 986.78 ms |
| `glm`::`mat4 * vec4` SIMD | 1017.16 ms |
| `lm`::`mat4 look_at` SIMD | 5473.76 ms |
| `glm`::`mat4 lookAt` SIMD | 4131.30 ms |

## x64 — Scalar (No SIMD)

| Function | lm | glm | linmath |
|---|---|---|---|
| `sqrtf` | 879.26 ms | — | — |
| `rsqrtf` | 438.53 ms | — | — |
| `vec3 dot` | 52.45 ms | 50.48 ms | 51.18 ms |
| `vec3 norm` | 1627.12 ms | 1163.73 ms | 102.05 ms |
| `vec3 cross` | 529.61 ms | 1138.32 ms | 252.55 ms |
| `mat4 translate` | 40.95 ms | 1745.55 ms | 1350.19 ms |
| `mat4 rotate_x` | 1926.75 ms | 8458.00 ms | 9467.67 ms |
| `mat4 mul` | 3410.49 ms | 4580.78 ms | 5408.38 ms |
| `mat4 * vec4` | 833.22 ms | 1014.19 ms | 1412.46 ms |
| `mat4 look_at` | 5450.31 ms | 4574.57 ms | 5866.87 ms |

## x86 — Scalar (No SIMD)

| Function | lm | glm | linmath |
|---|---|---|---|
| `sqrtf` | 2133.10 ms | — | — |
| `rsqrtf` | 1421.35 ms | — | — |
| `vec3 dot` | 50.63 ms | 51.20 ms | 50.62 ms |
| `vec3 norm` | 2484.36 ms | 2416.90 ms | 2175.95 ms |
| `vec3 cross` | 940.44 ms | 822.28 ms | 922.33 ms |
| `mat4 translate` | 1829.01 ms | 8256.54 ms | 2650.63 ms |
| `mat4 rotate_x` | 4825.54 ms | 20466.82 ms | 16719.11 ms |
| `mat4 mul` | 5777.06 ms | 13695.82 ms | 9128.60 ms |
| `mat4 * vec4` | 1142.84 ms | 3165.83 ms | 2201.41 ms |
| `mat4 look_at` | 13845.12 ms | 13605.16 ms | 14764.14 ms |

## x86 — SSE2

| Function | Time |
|---|---|
| `lm::sqrtf` SIMD | 826.78 ms |
| `lm::rsqrtf` SIMD | 580.39 ms |
| `lm::vec3 dot` SIMD | 50.68 ms |
| `glm::vec3 dot` SIMD | 53.31 ms |
| `lm::mat4 mul` SIMD | 4102.49 ms |
| `glm::mat4 mul` SIMD | 4582.56 ms |
| `lm::mat4 * vec4` SIMD | 1925.10 ms |
| `glm::mat4 * vec4` SIMD | 1922.73 ms |
| `lm::mat4 look_at` SIMD | 5945.23 ms |
| `glm::mat4 lookAt` SIMD | 6156.01 ms |

## x86 — AVX2

| Function | Time |
|---|---|
| `lm::sqrtf` SIMD | 741.63 ms |
| `lm::rsqrtf` SIMD | 489.42 ms |
| `lm::vec3 dot` SIMD | 50.91 ms |
| `glm::vec3 dot` SIMD | 53.05 ms |
| `lm::mat4 mul` SIMD | 2846.12 ms |
| `glm::mat4 mul` SIMD | 4065.42 ms |
| `lm::mat4 * vec4` SIMD | 1039.83 ms |
| `glm::mat4 * vec4` SIMD | 1787.38 ms |
| `lm::mat4 look_at` SIMD | 5749.46 ms |
| `glm::mat4 lookAt` SIMD | 6254.61 ms |

---

# Acknowledgements

This project would not exist without the original **linmath.h**.

Special thanks to its author for creating a minimal and elegant C math library that inspired this C++ implementation.

# License

See LICENSE file for details.
