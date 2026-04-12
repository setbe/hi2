#pragma once

#include "config.hpp"
#include "meta.hpp"

namespace a {
A_CONSTEXPR usize len(const char* s) noexcept {
    if (s == nullptr) {
        return 0;
    }
    const char* p = s;
    while (*p != '\0') {
        ++p;
    }
    return static_cast<usize>(p - s);
}

template<typename T>
A_CONSTEXPR remove_reference_t<T>&& move(T&& t) noexcept {
    return static_cast<remove_reference_t<T>&&>(t);
}

template<typename T>
A_CONSTEXPR T&& forward(remove_reference_t<T>& t) noexcept {
    return static_cast<T&&>(t);
}

template<typename T>
A_CONSTEXPR T&& forward(remove_reference_t<T>&& t) noexcept {
    return static_cast<T&&>(t);
}

template<typename T>
A_FORCE_INLINE T* launder_ptr(T* p) noexcept {
#if defined(__has_builtin)
#  if __has_builtin(__builtin_launder)
    return __builtin_launder(p);
#  endif
#endif
#if A_COMPILER_GCC && (__GNUC__ >= 7)
    return __builtin_launder(p);
#else
    return p;
#endif
}
} // namespace a
