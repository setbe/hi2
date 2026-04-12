// FIPS 180-4: Secure Hash Standard
// RFC 6234: Secure Hash Algorithms (SHA and SHA-based HMAC and HKDF)
// RFC 2104: HMAC (ipad=0x36, opad=0x5c, key normalize)
#pragma once

#include "../a.hpp"
#include "detail/wipe.hpp"

namespace a {
namespace crypto {
    struct sha384 {
        using u8 = a::u8;
        using u32 = a::u32;
        using u64 = a::u64;
        using usize = a::usize;

        using byte_view = a::byte_view;
        using byte_view_mut = a::byte_view_mut;
        template<usize N> using byte_view_n = a::byte_view_n<N>;
        template<usize N> using byte_view_mut_n = a::byte_view_mut_n<N>;

        // Constants
        static A_CONSTEXPR_VAR usize BLOCK_BYTES = 128; // 1024-bit blocks
        static A_CONSTEXPR_VAR usize DIGEST_BYTES = 48;  // 384-bit digest
        static A_CONSTEXPR_VAR usize HMAC_DIGEST_BYTES = 48;

        static_assert(BLOCK_BYTES == 128, "sha384: block is 128 bytes");
        static_assert(DIGEST_BYTES == 48, "sha384: digest is 48 bytes");
        static_assert(HMAC_DIGEST_BYTES <= DIGEST_BYTES, "sha384: HMAC out too large");
        static_assert(HMAC_DIGEST_BYTES >= 10, "sha384: <10 bytes SHA384 HMAC is insecure");

        // RAII
        explicit A_CONSTEXPR sha384() noexcept :
            _data{ 0 },
            _state{
                0xcbbb9d5dc1059ed8ULL, 0x629a292a367cd507ULL,
                0x9159015a3070dd17ULL, 0x152fecd8f70e5939ULL,
                0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL,
                0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL
            },
            _bitlen_lo{ 0 },
            _bitlen_hi{ 0 },
            _data_count{ 0 } { }
        ~sha384() noexcept { }

    private:
        void transform(byte_view block128) noexcept;
        void final_without_logical_reset(byte_view_mut out) noexcept;
        static void hmac(byte_view key, byte_view data, byte_view_mut out) noexcept;

        A_NODISCARD A_CONSTEXPR static u64 rotr(u64 x, u64 r) noexcept { return (x >> r) | (x << (64 - r)); }
        A_NODISCARD A_CONSTEXPR static u64 ch(u64 x, u64 y, u64 z) noexcept { return (x & y) ^ (~x & z); }
        A_NODISCARD A_CONSTEXPR static u64 major(u64 x, u64 y, u64 z) noexcept { return (x & y) ^ (x & z) ^ (y & z); }

        A_NODISCARD A_CONSTEXPR static u64 ep0(u64 x) noexcept { return rotr(x, 28) ^ rotr(x, 34) ^ rotr(x, 39); }
        A_NODISCARD A_CONSTEXPR static u64 ep1(u64 x) noexcept { return rotr(x, 14) ^ rotr(x, 18) ^ rotr(x, 41); }
        A_NODISCARD A_CONSTEXPR static u64 sig0(u64 x) noexcept { return rotr(x, 1) ^ rotr(x, 8) ^ (x >> 7); }
        A_NODISCARD A_CONSTEXPR static u64 sig1(u64 x) noexcept { return rotr(x, 19) ^ rotr(x, 61) ^ (x >> 6); }

        A_CONSTEXPR static void add_bits(u64& lo, u64& hi, u64 add) noexcept {
            u64 old = lo;
            lo += add;
            if (lo < old) ++hi;
        }

    public:
        // --------------------------------------------------------------------
        //                          Public API
        // --------------------------------------------------------------------

        // logical state reset for next reuse
        void reset() noexcept { *this = sha384{}; }

        void update(byte_view data) noexcept;
        template<usize N> void update(byte_view_n<N> data) noexcept { update((byte_view)data.v); }
        template<usize N> void update(const u8(&arr)[N]) noexcept { update(byte_view{ arr, N }); }

        template<usize N> void final_without_logical_reset(byte_view_mut_n<N> out) noexcept;
        template<usize N> void final_without_logical_reset(u8(&mutable_buffer)[N]) noexcept;

        template<usize N> void final(byte_view_mut_n<N> out) noexcept;
        template<usize N> void final(u8(&mutable_buffer)[N]) noexcept;

        template<usize NK, usize ND, usize NO>
        static void hmac(byte_view_n<NK> key,
            byte_view_n<ND> data,
            byte_view_mut_n<NO> out) noexcept {
            static_assert(NO >= DIGEST_BYTES, "hmac: out must be >= 48 bytes");
            hmac((byte_view)key.v, (byte_view)data.v, out.v);
        }

    private:
        u8  _data[BLOCK_BYTES]; // current block buffer
        u64 _state[8];          // (A..H)
        u64 _bitlen_lo;         // message length in bits (low 64)
        u64 _bitlen_hi;         // message length in bits (high 64)
        u32 _data_count;        // bytes currently in _data
    }; // struct sha384

    // SHA-384 constants (same K as SHA-512)
    static A_CONSTEXPR_VAR a::u64 SHA384_K[80]{
        0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
        0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
        0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
        0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
        0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
        0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
        0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
        0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
        0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
        0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
        0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
        0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
        0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
        0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
        0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
        0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
        0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
        0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
        0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
        0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
    };

    // ---- HMAC ----
    void sha384::hmac(byte_view key, byte_view data, byte_view_mut out) noexcept {
        u8 key_pad[BLOCK_BYTES]{ 0 };
        u8 o_pad[BLOCK_BYTES]{ 0 };
        u8 i_pad[BLOCK_BYTES]{ 0 };
        u8 inner_hash[DIGEST_BYTES]{ 0 };

        // ---- Step 1: normalize key ----
        if (key.size() > BLOCK_BYTES) {
            sha384 tmp;
            tmp.update(key);
            // write first 48 bytes into key_pad, rest remains 0
            byte_view_mut out48{ a::as_view_mut(key_pad).v.first(DIGEST_BYTES) };
            tmp.final_without_logical_reset(out48);
        }
        else {
            for (usize i = 0; i < key.size(); ++i) key_pad[i] = key[i];
        }

        // ---- Step 2: i_pad / o_pad ----
        for (usize i = 0; i < BLOCK_BYTES; ++i) {
            i_pad[i] = key_pad[i] ^ 0x36;
            o_pad[i] = key_pad[i] ^ 0x5c;
        }

        { // ---- Step 3: inner = hash(i_pad || data) ----
            sha384 inner;
            inner.update(byte_view{ i_pad, BLOCK_BYTES });
            inner.update(data);
            inner.final_without_logical_reset(a::as_view_mut(inner_hash));
        }

        { // ---- Step 4: outer = hash(o_pad || inner) ----
            sha384 outer;
            outer.update(byte_view{ o_pad, BLOCK_BYTES });
            outer.update(byte_view{ inner_hash, DIGEST_BYTES });
            outer.final_without_logical_reset(out);
        }
        // ---- Step 5: wipe secrets (best-effort) ----
        detail::secure_zero(key_pad, sizeof(key_pad));
        detail::secure_zero(i_pad, sizeof(i_pad));
        detail::secure_zero(o_pad, sizeof(o_pad));
        detail::secure_zero(inner_hash, sizeof(inner_hash));
    }

    // ---- Update ----
    void sha384::update(byte_view data) noexcept {
        if (!data) return; // treat nullptr/empty as no-op
        static_assert(sizeof(_data) == BLOCK_BYTES, "sha384: _data mismatch size");

        for (usize i = 0; i < data.size(); ++i) {
            _data[_data_count++] = data[i];
            if (_data_count == BLOCK_BYTES) {
                transform(byte_view{ _data, sizeof(_data) });
                add_bits(_bitlen_lo, _bitlen_hi, 1024); // 128 bytes * 8
                _data_count = 0;
            }
        }
    }

    // ---- Transform ----
    void sha384::transform(byte_view buf) noexcept {
        u64 a, b, c, d, e, f, g, h;
        u64 t1, t2;
        u64 m[80];

        // load big-endian 64-bit words
        usize j = 0;
        for (usize i = 0; i < 16; ++i, j += 8) {
            m[i] = (u64(buf[j + 0]) << 56) |
                (u64(buf[j + 1]) << 48) |
                (u64(buf[j + 2]) << 40) |
                (u64(buf[j + 3]) << 32) |
                (u64(buf[j + 4]) << 24) |
                (u64(buf[j + 5]) << 16) |
                (u64(buf[j + 6]) << 8) |
                (u64(buf[j + 7]) << 0);
        }
        for (usize i = 16; i < 80; ++i)
            m[i] = sig1(m[i - 2]) + m[i - 7] + sig0(m[i - 15]) + m[i - 16];

        a = _state[0]; b = _state[1]; c = _state[2]; d = _state[3];
        e = _state[4]; f = _state[5]; g = _state[6]; h = _state[7];

        for (usize i = 0; i < 80; ++i) {
            t1 = h + ep1(e) + ch(e, f, g) + SHA384_K[i] + m[i];
            t2 = ep0(a) + major(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        _state[0] += a; _state[1] += b; _state[2] += c; _state[3] += d;
        _state[4] += e; _state[5] += f; _state[6] += g; _state[7] += h;
    }

    // ---- Finalize (no reset) ----
    void sha384::final_without_logical_reset(byte_view_mut out) noexcept {
        static_assert(sizeof(_data) == BLOCK_BYTES, "sha384: _data mismatch size");
        usize i = _data_count;

        // Pad to 112 bytes (leave 16 bytes for 128-bit length)
        if (_data_count < 112) {
            _data[i++] = 0x80;
            while (i < 112) _data[i++] = 0;
        }
        else {
            _data[i++] = 0x80;
            while (i < 128) _data[i++] = 0;
            transform(byte_view{ _data, sizeof(_data) });
            for (usize k = 0; k < 112; ++k) _data[k] = 0;
        }

        // add message bits from remaining bytes
        add_bits(_bitlen_lo, _bitlen_hi, (u64)_data_count * 8ULL);

        // write 128-bit length big-endian into last 16 bytes:
        // [112..119] = hi, [120..127] = lo
        for (usize n = 0; n < 8; ++n) {
            _data[112 + n] = (u8)(_bitlen_hi >> (56 - 8 * n));
            _data[120 + n] = (u8)(_bitlen_lo >> (56 - 8 * n));
        }

        transform(byte_view{ _data, sizeof(_data) });

        // SHA-384 outputs first 6 state words (48 bytes)
        for (usize w = 0; w < 6; ++w) {
            out[w * 8 + 0] = (u8)(_state[w] >> 56);
            out[w * 8 + 1] = (u8)(_state[w] >> 48);
            out[w * 8 + 2] = (u8)(_state[w] >> 40);
            out[w * 8 + 3] = (u8)(_state[w] >> 32);
            out[w * 8 + 4] = (u8)(_state[w] >> 24);
            out[w * 8 + 5] = (u8)(_state[w] >> 16);
            out[w * 8 + 6] = (u8)(_state[w] >> 8);
            out[w * 8 + 7] = (u8)(_state[w] >> 0);
        }
    }

    template<a::usize N>
    void sha384::final_without_logical_reset(byte_view_mut_n<N> mutable_buffer) noexcept {
        static_assert(N >= DIGEST_BYTES, "sha384: out must be >= 48 bytes");
        final_without_logical_reset((byte_view_mut)mutable_buffer.v);
    }
    template<a::usize N>
    void sha384::final_without_logical_reset(u8(&mutable_buffer)[N]) noexcept {
        static_assert(N >= DIGEST_BYTES, "sha384: out must be >= 48 bytes");
        final_without_logical_reset(byte_view_mut{ mutable_buffer, N });
    }

    template<a::usize N>
    void sha384::final(byte_view_mut_n<N> mutable_buffer) noexcept {
        static_assert(N >= DIGEST_BYTES, "sha384: out must be >= 48 bytes");
        final_without_logical_reset((byte_view_mut)mutable_buffer.v);
        reset();
    }
    template<a::usize N>
    void sha384::final(u8(&mutable_buffer)[N]) noexcept {
        static_assert(N >= DIGEST_BYTES, "sha384: out must be >= 48 bytes");
        final_without_logical_reset(byte_view_mut{ mutable_buffer, N });
        reset();
    }

} // namespace crypto
} // namespace a
