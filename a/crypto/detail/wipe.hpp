#pragma once

#include "../../types.hpp"

namespace a {
namespace crypto {
namespace detail {
inline void secure_zero(void* p, a::usize n) noexcept {
    volatile a::u8* v = static_cast<volatile a::u8*>(p);
    while (n--) {
        *v++ = 0;
    }
}
} // namespace detail
} // namespace crypto
} // namespace a

