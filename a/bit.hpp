#pragma once

#include "config.hpp"
#include "types.hpp"

namespace a {
enum class endian : u8 {
    unknown = 0,
    little = 1,
    big = 2
};

A_NODISCARD A_CONSTEXPR endian native_endian() noexcept {
#if A_ENDIAN_LITTLE
    return endian::little;
#elif A_ENDIAN_BIG
    return endian::big;
#else
    return endian::unknown;
#endif
}

A_NODISCARD A_CONSTEXPR u16 bswap16(u16 x) noexcept {
    return static_cast<u16>((x >> 8) | (x << 8));
}

A_NODISCARD A_CONSTEXPR u32 bswap32(u32 x) noexcept {
    return ((x & 0x000000FFu) << 24) |
           ((x & 0x0000FF00u) << 8) |
           ((x & 0x00FF0000u) >> 8) |
           ((x & 0xFF000000u) >> 24);
}

A_NODISCARD A_CONSTEXPR u64 bswap64(u64 x) noexcept {
    return ((x & 0x00000000000000FFull) << 56) |
           ((x & 0x000000000000FF00ull) << 40) |
           ((x & 0x0000000000FF0000ull) << 24) |
           ((x & 0x00000000FF000000ull) << 8) |
           ((x & 0x000000FF00000000ull) >> 8) |
           ((x & 0x0000FF0000000000ull) >> 24) |
           ((x & 0x00FF000000000000ull) >> 40) |
           ((x & 0xFF00000000000000ull) >> 56);
}

A_NODISCARD A_CONSTEXPR u16 byteswap(u16 x) noexcept { return bswap16(x); }
A_NODISCARD A_CONSTEXPR u32 byteswap(u32 x) noexcept { return bswap32(x); }
A_NODISCARD A_CONSTEXPR u64 byteswap(u64 x) noexcept { return bswap64(x); }

A_NODISCARD A_CONSTEXPR u16 rotl16(u16 x, unsigned r) noexcept {
    r &= 15u;
    return static_cast<u16>((x << r) | (x >> ((16u - r) & 15u)));
}

A_NODISCARD A_CONSTEXPR u32 rotl32(u32 x, unsigned r) noexcept {
    r &= 31u;
    return static_cast<u32>((x << r) | (x >> ((32u - r) & 31u)));
}

A_NODISCARD A_CONSTEXPR u64 rotl64(u64 x, unsigned r) noexcept {
    r &= 63u;
    return static_cast<u64>((x << r) | (x >> ((64u - r) & 63u)));
}

A_NODISCARD A_CONSTEXPR u16 rotr16(u16 x, unsigned r) noexcept {
    r &= 15u;
    return static_cast<u16>((x >> r) | (x << ((16u - r) & 15u)));
}

A_NODISCARD A_CONSTEXPR u32 rotr32(u32 x, unsigned r) noexcept {
    r &= 31u;
    return static_cast<u32>((x >> r) | (x << ((32u - r) & 31u)));
}

A_NODISCARD A_CONSTEXPR u64 rotr64(u64 x, unsigned r) noexcept {
    r &= 63u;
    return static_cast<u64>((x >> r) | (x << ((64u - r) & 63u)));
}

template<usize Bits>
struct bitset {
    static_assert(Bits > 0, "bitset: Bits must be > 0");

    static A_CONSTEXPR_VAR usize word_bits = 64;
    static A_CONSTEXPR_VAR usize word_count = (Bits + (word_bits - 1)) / word_bits;

    u64 words[word_count] = {};

    void clear() noexcept {
        for (usize i = 0; i < word_count; ++i) {
            words[i] = 0;
        }
    }

    A_NODISCARD bool test(usize idx) const noexcept {
        if (idx >= Bits) {
            return false;
        }
        const usize w = idx / word_bits;
        const usize b = idx % word_bits;
        return (words[w] & (u64(1) << b)) != 0;
    }

    bool set(usize idx) noexcept {
        if (idx >= Bits) {
            return false;
        }
        const usize w = idx / word_bits;
        const usize b = idx % word_bits;
        words[w] |= (u64(1) << b);
        return true;
    }

    bool reset(usize idx) noexcept {
        if (idx >= Bits) {
            return false;
        }
        const usize w = idx / word_bits;
        const usize b = idx % word_bits;
        words[w] &= ~(u64(1) << b);
        return true;
    }

    A_NODISCARD bool any() const noexcept {
        for (usize i = 0; i < word_count; ++i) {
            if (words[i] != 0) {
                return true;
            }
        }
        return false;
    }

    A_NODISCARD usize count() const noexcept {
        usize out = 0;
        for (usize i = 0; i < word_count; ++i) {
            u64 x = words[i];
            while (x != 0) {
                x &= (x - 1);
                ++out;
            }
        }
        return out;
    }
};

template<typename T>
A_NODISCARD static inline T load_le(const void* src) noexcept = delete;

template<typename T>
A_NODISCARD static inline T load_be(const void* src) noexcept = delete;

template<typename T>
static inline void store_le(void* dst, T v) noexcept = delete;

template<typename T>
static inline void store_be(void* dst, T v) noexcept = delete;

template<>
A_NODISCARD inline u8 load_le<u8>(const void* src) noexcept {
    const u8* b = reinterpret_cast<const u8*>(src);
    return b[0];
}

template<>
A_NODISCARD inline u16 load_le<u16>(const void* src) noexcept {
    const u8* b = reinterpret_cast<const u8*>(src);
    return static_cast<u16>(static_cast<u16>(b[0]) | (static_cast<u16>(b[1]) << 8));
}

template<>
A_NODISCARD inline u32 load_le<u32>(const void* src) noexcept {
    const u8* b = reinterpret_cast<const u8*>(src);
    return static_cast<u32>(static_cast<u32>(b[0]) |
                            (static_cast<u32>(b[1]) << 8) |
                            (static_cast<u32>(b[2]) << 16) |
                            (static_cast<u32>(b[3]) << 24));
}

template<>
A_NODISCARD inline u64 load_le<u64>(const void* src) noexcept {
    const u8* b = reinterpret_cast<const u8*>(src);
    return static_cast<u64>(static_cast<u64>(b[0]) |
                            (static_cast<u64>(b[1]) << 8) |
                            (static_cast<u64>(b[2]) << 16) |
                            (static_cast<u64>(b[3]) << 24) |
                            (static_cast<u64>(b[4]) << 32) |
                            (static_cast<u64>(b[5]) << 40) |
                            (static_cast<u64>(b[6]) << 48) |
                            (static_cast<u64>(b[7]) << 56));
}

template<>
A_NODISCARD inline u8 load_be<u8>(const void* src) noexcept {
    const u8* b = reinterpret_cast<const u8*>(src);
    return b[0];
}

template<>
A_NODISCARD inline u16 load_be<u16>(const void* src) noexcept {
    const u8* b = reinterpret_cast<const u8*>(src);
    return static_cast<u16>((static_cast<u16>(b[0]) << 8) | static_cast<u16>(b[1]));
}

template<>
A_NODISCARD inline u32 load_be<u32>(const void* src) noexcept {
    const u8* b = reinterpret_cast<const u8*>(src);
    return static_cast<u32>((static_cast<u32>(b[0]) << 24) |
                            (static_cast<u32>(b[1]) << 16) |
                            (static_cast<u32>(b[2]) << 8) |
                            static_cast<u32>(b[3]));
}

template<>
A_NODISCARD inline u64 load_be<u64>(const void* src) noexcept {
    const u8* b = reinterpret_cast<const u8*>(src);
    return static_cast<u64>((static_cast<u64>(b[0]) << 56) |
                            (static_cast<u64>(b[1]) << 48) |
                            (static_cast<u64>(b[2]) << 40) |
                            (static_cast<u64>(b[3]) << 32) |
                            (static_cast<u64>(b[4]) << 24) |
                            (static_cast<u64>(b[5]) << 16) |
                            (static_cast<u64>(b[6]) << 8) |
                            static_cast<u64>(b[7]));
}

template<>
inline void store_le<u8>(void* dst, u8 v) noexcept {
    u8* b = reinterpret_cast<u8*>(dst);
    b[0] = v;
}

template<>
inline void store_le<u16>(void* dst, u16 v) noexcept {
    u8* b = reinterpret_cast<u8*>(dst);
    b[0] = static_cast<u8>(v);
    b[1] = static_cast<u8>(v >> 8);
}

template<>
inline void store_le<u32>(void* dst, u32 v) noexcept {
    u8* b = reinterpret_cast<u8*>(dst);
    b[0] = static_cast<u8>(v);
    b[1] = static_cast<u8>(v >> 8);
    b[2] = static_cast<u8>(v >> 16);
    b[3] = static_cast<u8>(v >> 24);
}

template<>
inline void store_le<u64>(void* dst, u64 v) noexcept {
    u8* b = reinterpret_cast<u8*>(dst);
    b[0] = static_cast<u8>(v);
    b[1] = static_cast<u8>(v >> 8);
    b[2] = static_cast<u8>(v >> 16);
    b[3] = static_cast<u8>(v >> 24);
    b[4] = static_cast<u8>(v >> 32);
    b[5] = static_cast<u8>(v >> 40);
    b[6] = static_cast<u8>(v >> 48);
    b[7] = static_cast<u8>(v >> 56);
}

template<>
inline void store_be<u8>(void* dst, u8 v) noexcept {
    u8* b = reinterpret_cast<u8*>(dst);
    b[0] = v;
}

template<>
inline void store_be<u16>(void* dst, u16 v) noexcept {
    u8* b = reinterpret_cast<u8*>(dst);
    b[0] = static_cast<u8>(v >> 8);
    b[1] = static_cast<u8>(v);
}

template<>
inline void store_be<u32>(void* dst, u32 v) noexcept {
    u8* b = reinterpret_cast<u8*>(dst);
    b[0] = static_cast<u8>(v >> 24);
    b[1] = static_cast<u8>(v >> 16);
    b[2] = static_cast<u8>(v >> 8);
    b[3] = static_cast<u8>(v);
}

template<>
inline void store_be<u64>(void* dst, u64 v) noexcept {
    u8* b = reinterpret_cast<u8*>(dst);
    b[0] = static_cast<u8>(v >> 56);
    b[1] = static_cast<u8>(v >> 48);
    b[2] = static_cast<u8>(v >> 40);
    b[3] = static_cast<u8>(v >> 32);
    b[4] = static_cast<u8>(v >> 24);
    b[5] = static_cast<u8>(v >> 16);
    b[6] = static_cast<u8>(v >> 8);
    b[7] = static_cast<u8>(v);
}

A_NODISCARD static inline bool double_is_nan(double x) noexcept {
    union {
        double d;
        u64 u;
    } v{x};
    const u64 exp = (v.u >> 52) & 0x7FFull;
    const u64 frac = v.u & 0x000FFFFFFFFFFFFFull;
    return (exp == 0x7FFull) && (frac != 0);
}

A_NODISCARD static inline bool double_is_inf(double x) noexcept {
    union {
        double d;
        u64 u;
    } v{x};
    const u64 exp = (v.u >> 52) & 0x7FFull;
    const u64 frac = v.u & 0x000FFFFFFFFFFFFFull;
    return (exp == 0x7FFull) && (frac == 0);
}
} // namespace a
