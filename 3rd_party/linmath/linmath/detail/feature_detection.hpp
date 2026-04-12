#pragma once

// -------------------- C++ standard detection --------------------------------

// MSVC
#if defined(_MSC_VER)
#   define COMPILER_MSVC _MSC_VER

#   if defined(_MSVC_LANG)
#       define CPP_VERSION _MSVC_LANG
#   else
#       define CPP_VERSION __cplusplus
#   endif

// Clang
#elif defined(__clang__)
#   define CPP_VERSION __cplusplus

// GCC
#elif defined(__GNUC__)
#   define CPP_VERSION __cplusplus

#else
#   define CPP_VERSION __cplusplus
#endif

// -------------------- Feature flags -----------------------------------------

#if CPP_VERSION >= 202002L
#   define LMATH_CXX20
#   define LMATH_CXX17
#elif CPP_VERSION >= 201703L
#   define LMATH_CXX17
#endif

// -------------------- Force inline ------------------------------------------

#if defined(COMPILER_MSVC)
#   define LMATH_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#   define LMATH_FORCE_INLINE inline __attribute__((always_inline))
#else
#   define LMATH_FORCE_INLINE inline
#endif

// -------------------- Attribute / constexpr macros --------------------------

#if defined(LMATH_CXX17)
#   define LMATH_NO_DISCARD    [[nodiscard]]
#   define LMATH_CONSTEXPR     constexpr
#   define LMATH_CONSTEXPR_VAR constexpr
#else
#   define LMATH_NO_DISCARD
#   define LMATH_CONSTEXPR     inline
#   define LMATH_CONSTEXPR_VAR static const
#endif

// -------------------- consteval compatibility -------------------------------
//
// C++20 : real consteval
// C++17 : constexpr (best possible)
// <C++17: inline fallback
//
#if defined(LMATH_CXX20)
#   define LMATH_CONSTEVAL consteval
#elif defined(LMATH_CXX17)
#   define LMATH_CONSTEVAL constexpr
#else
#   define LMATH_CONSTEVAL inline
#endif

// -------------------- Function annotation -----------------------------------

#if defined(LMATH_NO_DISCARD)
#   define LMATH_OUT LMATH_NO_DISCARD LMATH_CONSTEXPR
#else
#   define LMATH_OUT LMATH_CONSTEXPR
#endif
