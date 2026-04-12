#pragma once

#include "config.hpp"
#include "view.hpp"

namespace a {
template<usize N>
A_NODISCARD A_CONSTEXPR char_view u8view(const char (&s)[N]) noexcept {
    return char_view{s, N ? N - 1 : 0};
}

#if A_HAS_CHAR8_T
template<usize N>
A_NODISCARD A_CONSTEXPR char_view u8view(const char8_t (&s)[N]) noexcept {
    return char_view{reinterpret_cast<const char*>(s), N ? N - 1 : 0};
}
#endif
} // namespace a

#define A_U8(s) ::a::u8view(u8##s)
