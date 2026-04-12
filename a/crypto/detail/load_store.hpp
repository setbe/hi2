#pragma once

#include "../../bit.hpp"
#include "../../types.hpp"

namespace a {
namespace crypto {
namespace detail {
A_NODISCARD inline a::u32 load32_le(const a::u8* p) noexcept {
    return a::load_le<a::u32>(p);
}

A_NODISCARD inline a::u64 load64_le(const a::u8* p) noexcept {
    return a::load_le<a::u64>(p);
}

A_NODISCARD inline a::u32 load32_be(const a::u8* p) noexcept {
    return a::load_be<a::u32>(p);
}

A_NODISCARD inline a::u64 load64_be(const a::u8* p) noexcept {
    return a::load_be<a::u64>(p);
}

inline void store32_le(a::u8* p, a::u32 v) noexcept {
    a::store_le<a::u32>(p, v);
}

inline void store64_le(a::u8* p, a::u64 v) noexcept {
    a::store_le<a::u64>(p, v);
}

inline void store32_be(a::u8* p, a::u32 v) noexcept {
    a::store_be<a::u32>(p, v);
}

inline void store64_be(a::u8* p, a::u64 v) noexcept {
    a::store_be<a::u64>(p, v);
}
} // namespace detail
} // namespace crypto
} // namespace a

