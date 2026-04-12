#pragma once

#include "config.hpp"
#include "contract.hpp"
#include "meta.hpp"
#include "types.hpp"

namespace a {
template<typename T, T Min, T Max>
struct pow2_bounded;

template<typename T, T V, T Min, T Max>
struct static_bounded;

namespace detail {
template<typename T>
struct is_unsigned_integer
    : constant<bool,
               is_same_v<T, u8> || is_same_v<T, u16> || is_same_v<T, u32> || is_same_v<T, u64> ||
                   is_same_v<T, usize>> {};

template<typename T>
struct is_bounded_scalar
    : constant<bool,
               is_same_v<T, i8> || is_same_v<T, u8> || is_same_v<T, i16> || is_same_v<T, u16> ||
                   is_same_v<T, i32> || is_same_v<T, u32> || is_same_v<T, i64> ||
                   is_same_v<T, u64> || is_same_v<T, isize> || is_same_v<T, usize>> {};
} // namespace detail

template<typename T, T Min, T Max>
struct bounded {
    static_assert(detail::is_bounded_scalar<T>::value,
                  "a::bounded: T must be one of a::{i/u}{8,16,32,64}, a::isize, a::usize");
    static_assert(!(Max < Min), "a::bounded: Min must be <= Max");

    using value_type = T;
    static A_CONSTEXPR_VAR T min_value = Min;
    static A_CONSTEXPR_VAR T max_value = Max;

    // Runtime constrained value: constructor/set enforce [Min, Max].
    T value = Min;

    A_CONSTEXPR bounded() noexcept = default;

    A_CONSTEXPR explicit bounded(T v) noexcept
        : value(v) {
        contract::require(in_range(v));
    }

    A_NODISCARD static A_CONSTEXPR bool in_range(T v) noexcept {
        return !(v < Min) && !(Max < v);
    }

    A_NODISCARD static A_CONSTEXPR bool try_make(T v, bounded& out) noexcept {
        if (!in_range(v)) {
            return false;
        }
        out = make_trusted(v);
        return true;
    }

    A_CONSTEXPR void set(T v) noexcept {
        contract::require(in_range(v));
        value = v;
    }

    A_NODISCARD A_CONSTEXPR T get() const noexcept {
        return value;
    }

    A_NODISCARD A_CONSTEXPR explicit operator T() const noexcept {
        return value;
    }

private:
    template<typename U, U MinU, U MaxU>
    friend struct pow2_bounded;

    template<typename U, U VU, U MinU, U MaxU>
    friend struct static_bounded;

    struct trusted_tag {};

    A_NODISCARD static A_CONSTEXPR bounded make_trusted(T v) noexcept {
        return bounded(v, trusted_tag{});
    }

    A_CONSTEXPR bounded(T v, trusted_tag) noexcept
        : value(v) {}
};

template<typename T>
A_NODISCARD A_CONSTEXPR bool is_pow2(T v) noexcept {
    static_assert(detail::is_unsigned_integer<T>::value,
                  "a::is_pow2<T>: T must be an unsigned integer type");
    return (v > static_cast<T>(0)) && ((v & (v - static_cast<T>(1))) == static_cast<T>(0));
}

template<typename T, T Min, T Max>
struct pow2_bounded {
    static_assert(detail::is_unsigned_integer<T>::value,
                  "a::pow2_bounded: T must be an unsigned integer type");
    static_assert(is_pow2(Min), "a::pow2_bounded: Min must be power-of-two");
    static_assert(is_pow2(Max), "a::pow2_bounded: Max must be power-of-two");

    using bounded_type = bounded<T, Min, Max>;
    using value_type = T;

    bounded_type value = bounded_type::make_trusted(Min);

    A_CONSTEXPR pow2_bounded() noexcept = default;

    A_CONSTEXPR explicit pow2_bounded(T v) noexcept
        : value(bounded_type::make_trusted(Min)) {
        contract::require(in_range(v));
        value = bounded_type::make_trusted(v);
    }

    A_NODISCARD static A_CONSTEXPR bool in_range(T v) noexcept {
        return bounded_type::in_range(v) && is_pow2(v);
    }

    A_NODISCARD static A_CONSTEXPR bool try_make(T v, pow2_bounded& out) noexcept {
        if (!in_range(v)) {
            return false;
        }
        out = make_trusted(v);
        return true;
    }

    A_CONSTEXPR void set(T v) noexcept {
        contract::require(in_range(v));
        value = bounded_type::make_trusted(v);
    }

    A_NODISCARD A_CONSTEXPR T get() const noexcept {
        return value.get();
    }

    A_NODISCARD A_CONSTEXPR explicit operator T() const noexcept {
        return value.get();
    }

private:
    struct trusted_tag {};

    A_NODISCARD static A_CONSTEXPR pow2_bounded make_trusted(T v) noexcept {
        return pow2_bounded(v, trusted_tag{});
    }

    A_CONSTEXPR pow2_bounded(T v, trusted_tag) noexcept
        : value(bounded_type::make_trusted(v)) {}
};

template<typename T, T V, T Min, T Max>
struct static_bounded {
    static_assert(bounded<T, Min, Max>::in_range(V),
                  "a::static_bounded: V must be inside [Min, Max]");

    using value_type = T; // Compile-time constrained constant (no runtime check path).
    using bounded_type = bounded<T, Min, Max>;

    static A_CONSTEXPR_VAR T value = V;

    A_NODISCARD static A_CONSTEXPR bounded_type make() noexcept {
        return bounded_type::make_trusted(V);
    }
};
} // namespace a
