#pragma once

#include "config.hpp"
#include "types.hpp"

namespace a {
template<bool B, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T> {
    using type = T;
};

template<bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

template<typename T, T V>
struct constant {
    static A_CONSTEXPR_VAR T value = V;
    using type = constant;
    A_CONSTEXPR operator T() const noexcept { return value; }
};

using true_t = constant<bool, true>;
using false_t = constant<bool, false>;

template<typename A, typename B>
struct is_same : false_t {};

template<typename A>
struct is_same<A, A> : true_t {};

template<typename A, typename B>
A_CONSTEXPR_VAR bool is_same_v = is_same<A, B>::value;

template<typename...>
using void_t = void;

template<typename T>
struct is_void : false_t {};

template<>
struct is_void<void> : true_t {};

template<typename T>
A_CONSTEXPR_VAR bool is_void_v = is_void<T>::value;

template<typename T>
struct remove_reference {
    using type = T;
};

template<typename T>
struct remove_reference<T&> {
    using type = T;
};

template<typename T>
struct remove_reference<T&&> {
    using type = T;
};

template<typename T>
using remove_reference_t = typename remove_reference<T>::type;

#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
template<typename T>
struct is_nothrow_destructible : constant<bool, __is_nothrow_destructible(T)> {};

template<typename T, typename... Args>
struct is_nothrow_constructible : constant<bool, __is_nothrow_constructible(T, Args...)> {};
#else
#  error "a::nothrow traits require compiler builtin support"
#endif

template<typename T>
A_CONSTEXPR_VAR bool is_nothrow_destructible_v = is_nothrow_destructible<T>::value;

template<typename T, typename... Args>
A_CONSTEXPR_VAR bool is_nothrow_constructible_v = is_nothrow_constructible<T, Args...>::value;
} // namespace a
