#pragma once

#if defined(_MSC_VER)
#  define A_COMPILER_MSVC 1
#else
#  define A_COMPILER_MSVC 0
#endif

#if defined(__clang__)
#  define A_COMPILER_CLANG 1
#else
#  define A_COMPILER_CLANG 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
#  define A_COMPILER_GCC 1
#else
#  define A_COMPILER_GCC 0
#endif

#if defined(_M_IX86) || defined(__i386__)
#  define A_ARCH_X86_32 1
#else
#  define A_ARCH_X86_32 0
#endif

#if defined(_M_X64) || defined(__x86_64__)
#  define A_ARCH_X86_64 1
#else
#  define A_ARCH_X86_64 0
#endif

#if (A_ARCH_X86_32 || A_ARCH_X86_64)
#  define A_ARCH_X86 1
#else
#  define A_ARCH_X86 0
#endif

#if defined(_M_ARM) || defined(__arm__)
#  define A_ARCH_ARM32 1
#else
#  define A_ARCH_ARM32 0
#endif

#if defined(_M_ARM64) || defined(__aarch64__)
#  define A_ARCH_ARM64 1
#else
#  define A_ARCH_ARM64 0
#endif

#if (A_ARCH_ARM32 || A_ARCH_ARM64)
#  define A_ARCH_ARM 1
#else
#  define A_ARCH_ARM 0
#endif

#if (!A_ARCH_X86 && !A_ARCH_ARM)
#  define A_ARCH_UNKNOWN 1
#else
#  define A_ARCH_UNKNOWN 0
#endif

#if A_COMPILER_MSVC
#  if defined(_MSVC_LANG)
#    define A_CXX_VERSION _MSVC_LANG
#  else
#    define A_CXX_VERSION __cplusplus
#  endif
#else
#  define A_CXX_VERSION __cplusplus
#endif

#if A_CXX_VERSION >= 201703L
#  define A_CXX_17 1
#else
#  define A_CXX_17 0
#endif

#if A_CXX_VERSION >= 202002L
#  define A_CXX_20 1
#else
#  define A_CXX_20 0
#endif

#if defined(__cpp_char8_t) && (__cpp_char8_t >= 201811L)
#  define A_HAS_CHAR8_T 1
#else
#  define A_HAS_CHAR8_T 0
#endif

#if defined(__SIZEOF_INT128__)
#  define A_HAS_INT128 1
#else
#  define A_HAS_INT128 0
#endif

// MSVC targets supported by this project are treated as little-endian.
#if A_COMPILER_MSVC
#  define A_ENDIAN_LITTLE 1
#  define A_ENDIAN_BIG 0
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#  define A_ENDIAN_LITTLE 1
#  define A_ENDIAN_BIG 0
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#  define A_ENDIAN_LITTLE 0
#  define A_ENDIAN_BIG 1
#else
#  define A_ENDIAN_LITTLE 0
#  define A_ENDIAN_BIG 0
#endif

#if A_CXX_VERSION < 202302L
#  ifndef A_RESTRICT
#    if defined(__GNUC__)
#      define A_RESTRICT __restrict__
#    elif A_COMPILER_MSVC
#      define A_RESTRICT __restrict
#    else
#      define A_RESTRICT
#    endif
#  endif
#else
#  ifndef A_RESTRICT
#    define A_RESTRICT
#  endif
#endif

#if A_CXX_17
#  ifndef A_NODISCARD
#    define A_NODISCARD [[nodiscard]]
#  endif
#  ifndef A_CONSTEXPR
#    define A_CONSTEXPR constexpr
#  endif
#  ifndef A_CONSTEXPR_VAR
#    define A_CONSTEXPR_VAR constexpr
#  endif
#else
#  ifndef A_NODISCARD
#    define A_NODISCARD
#  endif
#  ifndef A_CONSTEXPR
#    define A_CONSTEXPR constexpr
#  endif
#  ifndef A_CONSTEXPR_VAR
#    define A_CONSTEXPR_VAR const
#  endif
#endif

#if !defined(A_FORCE_INLINE)
#  if A_COMPILER_MSVC
#    define A_FORCE_INLINE __forceinline
#  elif A_COMPILER_CLANG || A_COMPILER_GCC
#    define A_FORCE_INLINE inline __attribute__((always_inline))
#  else
#    define A_FORCE_INLINE inline
#  endif
#endif

#if !defined(A_NOINLINE)
#  if A_COMPILER_MSVC
#    define A_NOINLINE __declspec(noinline)
#  elif A_COMPILER_CLANG || A_COMPILER_GCC
#    define A_NOINLINE __attribute__((noinline))
#  else
#    define A_NOINLINE
#  endif
#endif

#if !defined(A_ALIGNAS)
#  define A_ALIGNAS(N) alignas(N)
#endif

#if !defined(A_CACHELINE)
// Conservative default cache line size used by many desktop/server CPUs.
// Can be overridden per target/toolchain when needed.
#  define A_CACHELINE 64
#endif

#if !defined(A_LIKELY)
#  if A_COMPILER_CLANG || A_COMPILER_GCC
#    define A_LIKELY(x) __builtin_expect(!!(x), 1)
#  else
#    define A_LIKELY(x) (x)
#  endif
#endif

#if !defined(A_UNLIKELY)
#  if A_COMPILER_CLANG || A_COMPILER_GCC
#    define A_UNLIKELY(x) __builtin_expect(!!(x), 0)
#  else
#    define A_UNLIKELY(x) (x)
#  endif
#endif

#if !defined(A_FALLTHROUGH)
#  if A_CXX_17
#    define A_FALLTHROUGH [[fallthrough]]
#  elif A_COMPILER_CLANG || A_COMPILER_GCC
#    define A_FALLTHROUGH __attribute__((fallthrough))
#  else
#    define A_FALLTHROUGH ((void)0)
#  endif
#endif
