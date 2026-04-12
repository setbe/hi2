#pragma once

#include "../../view.hpp"

namespace a {
namespace crypto {
namespace detail {
template<a::usize N>
A_NODISCARD A_CONSTEXPR bool ct_equal_n(a::byte_view_n<N> a, a::byte_view_n<N> b) noexcept {
    a::u8 diff = 0;
    for (a::usize i = 0; i < N; ++i) {
        diff |= static_cast<a::u8>(a.v[i] ^ b.v[i]);
    }
    return diff == 0;
}

A_NODISCARD inline bool ct_equal(a::byte_view a, a::byte_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    a::u8 diff = 0;
    for (a::usize i = 0; i < a.size(); ++i) {
        diff |= static_cast<a::u8>(a[i] ^ b[i]);
    }
    return diff == 0;
}
} // namespace detail
} // namespace crypto
} // namespace a

