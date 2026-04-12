#pragma once

#include "config.hpp"
#include "utility.hpp"

namespace a {
template<typename T>
struct view {
    using value_type = T;

    A_CONSTEXPR view() noexcept : _ptr(nullptr), _len(0) {}
    A_CONSTEXPR view(T* ptr, usize len) noexcept : _ptr(ptr), _len(len) {}

    template<typename U = T, typename P, typename = enable_if_t<is_same_v<U, const char> && is_same_v<P, const char*>>>
    A_CONSTEXPR view(P ptr) noexcept : _ptr(ptr), _len(::a::len(ptr)) {}

    template<usize N, typename U = T, typename = enable_if_t<!is_same_v<U, const char>>>
    A_CONSTEXPR view(U (&arr)[N]) noexcept : _ptr(arr), _len(N) {}

    template<usize N, typename U = T, typename = enable_if_t<is_same_v<U, const char>>>
    A_CONSTEXPR view(const char (&arr)[N]) noexcept : _ptr(arr), _len((N > 0 && arr[N - 1] == '\0') ? (N - 1) : N) {}

    A_CONSTEXPR T& operator[](usize i) const noexcept { return _ptr[i]; }
    A_NODISCARD A_CONSTEXPR T* data() const noexcept { return _ptr; }
    A_NODISCARD A_CONSTEXPR usize size() const noexcept { return _len; }
    A_NODISCARD A_CONSTEXPR bool empty() const noexcept { return _len == 0; }
    A_CONSTEXPR explicit operator bool() const noexcept { return (_ptr != nullptr) && (_len != 0); }

    A_CONSTEXPR T* begin() const noexcept { return _ptr; }
    A_CONSTEXPR T* end() const noexcept { return _ptr + _len; }

    A_CONSTEXPR T& front() const noexcept { return _ptr[0]; }
    A_CONSTEXPR T& back() const noexcept { return _ptr[_len - 1]; }

    A_NODISCARD A_CONSTEXPR view first(usize n) const noexcept { return view(_ptr, (n <= _len ? n : _len)); }
    A_NODISCARD A_CONSTEXPR view last(usize n) const noexcept { return view(_ptr + (_len > n ? _len - n : 0), (n <= _len ? n : _len)); }

    A_NODISCARD A_CONSTEXPR view drop(usize n) const noexcept {
        if (n >= _len) {
            return view();
        }
        return view(_ptr + n, _len - n);
    }

    A_NODISCARD A_CONSTEXPR view slice(usize pos, usize count) const noexcept {
        if (pos >= _len) {
            return view();
        }
        usize r = _len - pos;
        if (count < r) {
            r = count;
        }
        return view(_ptr + pos, r);
    }

    A_NODISCARD A_CONSTEXPR view subview(usize pos, usize count) const noexcept { return slice(pos, count); }

    A_NODISCARD A_CONSTEXPR usize find(const T& value, usize pos = 0) const noexcept {
        if (pos >= _len) {
            return npos;
        }
        for (usize i = pos; i < _len; ++i) {
            if (_ptr[i] == value) {
                return i;
            }
        }
        return npos;
    }

    A_NODISCARD A_CONSTEXPR usize find(view needle, usize pos = 0) const noexcept {
        if (needle._len == 0) {
            return pos <= _len ? pos : npos;
        }
        if (needle._len > _len || pos > (_len - needle._len)) {
            return npos;
        }
        for (usize i = pos; i <= (_len - needle._len); ++i) {
            usize j = 0;
            for (; j < needle._len; ++j) {
                if (!(_ptr[i + j] == needle._ptr[j])) {
                    break;
                }
            }
            if (j == needle._len) {
                return i;
            }
        }
        return npos;
    }

    A_NODISCARD A_CONSTEXPR usize find(const T* needle, usize needle_len, usize pos = 0) const noexcept {
        if (needle == nullptr) {
            return npos;
        }
        return find(view(needle, needle_len), pos);
    }

    friend A_CONSTEXPR bool operator==(view a, view b) noexcept {
        if (a._len != b._len) {
            return false;
        }
        for (usize i = 0; i < a._len; ++i) {
            if (!(a._ptr[i] == b._ptr[i])) {
                return false;
            }
        }
        return true;
    }

    friend A_CONSTEXPR bool operator!=(view a, view b) noexcept { return !(a == b); }

    template<usize N, typename U = T, typename = enable_if_t<is_same_v<U, const char>>>
    friend A_CONSTEXPR bool operator==(view a, const char (&lit)[N]) noexcept {
        return a == view(lit);
    }

    template<usize N, typename U = T, typename = enable_if_t<is_same_v<U, const char>>>
    friend A_CONSTEXPR bool operator!=(view a, const char (&lit)[N]) noexcept {
        return !(a == lit);
    }

    template<usize N, typename U = T, typename = enable_if_t<is_same_v<U, const char>>>
    friend A_CONSTEXPR bool operator==(const char (&lit)[N], view b) noexcept {
        return b == lit;
    }

    template<usize N, typename U = T, typename = enable_if_t<is_same_v<U, const char>>>
    friend A_CONSTEXPR bool operator!=(const char (&lit)[N], view b) noexcept {
        return !(b == lit);
    }

private:
    T* _ptr;
    usize _len;
};

template<typename T, usize N>
struct view_n {
    view<T> v;
    static A_CONSTEXPR_VAR usize size = N;

    // Fail-safe construction from dynamic view: only valid when sizes match exactly.
    explicit view_n(view<T> other) noexcept
        : v((other.size() == N) ? other.data() : static_cast<T*>(nullptr), (other.size() == N) ? N : 0) {}
    explicit view_n(T* data) noexcept : v(data, N) {}

    A_NODISCARD A_CONSTEXPR T* data() const noexcept { return v.data(); }
    A_NODISCARD A_CONSTEXPR usize len() const noexcept { return v.size(); }
    A_NODISCARD A_CONSTEXPR operator view<T>() const noexcept { return v; }
};

template<usize N>
using char_view_n = view_n<const char, N>;
template<usize N>
using char_view_mut_n = view_n<char, N>;

using char_view = view<const char>;
using char_view_mut = view<char>;

template<usize N>
using byte_view_n = view_n<const u8, N>;
template<usize N>
using byte_view_mut_n = view_n<u8, N>;

using byte_view = view<const u8>;
using byte_view_mut = view<u8>;

template<typename T, usize N>
A_CONSTEXPR view_n<const T, N> as_view(const T (&a)[N]) noexcept {
    return view_n<const T, N>(view<const T>{a, N});
}

template<typename T, usize N>
A_CONSTEXPR view_n<const T, N> as_view(view_n<T, N> v) noexcept {
    return view_n<const T, N>(view<const T>{v.data(), N});
}

template<typename T, usize N>
A_CONSTEXPR view_n<T, N> as_view_mut(T (&a)[N]) noexcept {
    return view_n<T, N>(view<T>{a, N});
}
} // namespace a
