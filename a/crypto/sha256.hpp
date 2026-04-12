// FIPS 180-4: Secure Hash Standard
// RFC 6234: Secure Hash Algorithms (SHA and SHA-based HMAC and HKDF)
// RFC 2104: HMAC (ipad=0x36, opad=0x5c, key normalize)
#pragma once

#include "../a.hpp"
#include "detail/wipe.hpp"

namespace a {
namespace crypto {
    struct sha256 {
        using u8 = a::u8;
        using u32 = a::u32;
        using usize = a::usize;
        using byte_view     = a::byte_view;
        using byte_view_mut = a::byte_view_mut;
        template<usize N> using byte_view_n     = a::byte_view_n<N>;
        template<usize N> using byte_view_mut_n = a::byte_view_mut_n<N>;
        // Constants
        static A_CONSTEXPR_VAR usize BLOCK_BYTES = 64;
        static A_CONSTEXPR_VAR usize DIGEST_BYTES = 32;
        static A_CONSTEXPR_VAR usize HMAC_DIGEST_BYTES = 32;
        static_assert(BLOCK_BYTES >= 32, "sha256: BLOCK_BYTES expected to be >= 32 bytes");
        static_assert(DIGEST_BYTES == 32, "sha256: digest is 32 bytes");
        static_assert(HMAC_DIGEST_BYTES <= DIGEST_BYTES, "sha256: HMAC out too large");
        static_assert(HMAC_DIGEST_BYTES >= 10, "sha256: <10 bytes SHA256 HMAC is insecure");

        // RAII
        explicit A_CONSTEXPR sha256() noexcept :
            _data{ 0 },
            _state{ 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 },
            _bit_length{ 0 },
            _data_count{ 0 } { }
        ~sha256() noexcept { }

    private:
        void transform(byte_view data) noexcept;
        void final_without_logical_reset(byte_view_mut out) noexcept;
        static void hmac(byte_view key, byte_view data, byte_view_mut out) noexcept;

        A_NODISCARD A_CONSTEXPR static u32 rotr(u32 a, u32 b) noexcept { return (a >> b) | (a << (32 - b)); }
        A_NODISCARD A_CONSTEXPR static u32 ch(u32 x, u32 y, u32 z) noexcept { return (x & y) ^ (~x & z); }
        A_NODISCARD A_CONSTEXPR static u32 major(u32 x, u32 y, u32 z) noexcept { return (x & y) ^ (x & z) ^ (y & z); }
        A_NODISCARD A_CONSTEXPR static u32 ep0(u32 x) noexcept { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
        A_NODISCARD A_CONSTEXPR static u32 ep1(u32 x) noexcept { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
        A_NODISCARD A_CONSTEXPR static u32 sig0(u32 x) noexcept { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
        A_NODISCARD A_CONSTEXPR static u32 sig1(u32 x) noexcept { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    public:
        // --------------------------------------------------------------------
        //                    Public API
        // --------------------------------------------------------------------

        // logical state reset for next reuse
        void reset() noexcept { *this = sha256{}; }

        void update(byte_view data) noexcept;
        template<usize N> void update(byte_view_n<N> data) noexcept { update((byte_view)data.v); }
        template<usize N> void update(const u8(&arr)[N]) noexcept { update(byte_view{ arr, N }); }

        // Finalize and produce the SHA-256 hash
        template<usize N> void final_without_logical_reset(byte_view_mut_n<N> out) noexcept;
        template<usize N> void final_without_logical_reset(u8(&mutable_buffer)[N]) noexcept;

        // Finalize and produce the SHA-256 hash, then RESET logical state
        template<usize N> void final(byte_view_mut_n<N> out) noexcept;
        template<usize N> void final(u8(&mutable_buffer)[N]) noexcept;

        template<usize NK, usize ND, usize NO>
        static void hmac(byte_view_n<NK> key,
                         byte_view_n<ND> data,
                         byte_view_mut_n<NO> out) noexcept {
            static_assert(NO >= DIGEST_BYTES, "hmac: out must be >= 32 bytes");
            hmac((byte_view)key.v, (byte_view)data.v, out.v);
        }

    private:
        u8 _data[BLOCK_BYTES]; // Input data block being processed
        u32 _state[8];         // State (A, B, C, D, E, F, G, H)
        a::u64 _bit_length;       // Total length of the message in bits
        u32 _data_count;
    }; // struct sha256

    void sha256::hmac(byte_view key, byte_view data, byte_view_mut out) noexcept {
        u8 key_pad[BLOCK_BYTES]{0};
        u8 o_pad[BLOCK_BYTES]{0};
        u8 i_pad[BLOCK_BYTES]{0};
        u8 inner_hash[HMAC_DIGEST_BYTES]{0};

        // ---- Step 1: normalize key ----
        if (key.size() > BLOCK_BYTES) {
            sha256 tmp;
            tmp.update(key);
            byte_view_mut out32{ a::as_view_mut(key_pad).v.first(32) }; // trunc to first 32 bytes
            tmp.final_without_logical_reset(out32); // copy directly to `key_pad`
            // Others elements already filled with zeroes
        }
        else {
            for (usize i=0; i < key.size(); ++i)
                key_pad[i] = key[i];
            // Others elements already filled with zeroes
        }
        // ---- Step 2: i_pad / o_pad ----
        for (usize i=0; i < BLOCK_BYTES; ++i) {
            i_pad[i] = key_pad[i] ^ 0x36;
            o_pad[i] = key_pad[i] ^ 0x5c;
        }
        { // ---- Step 3: inner = hash(i_pad || data) ----
            sha256 inner;
            inner.update(byte_view{ i_pad, BLOCK_BYTES });
            inner.update(data);
            inner.final_without_logical_reset(a::as_view_mut(inner_hash));
        }
        { // ---- Step 4: outer = hash(o_pad || inner) ----
            sha256 outer;
            outer.update(byte_view{ o_pad, BLOCK_BYTES });
            outer.update(byte_view{ inner_hash, HMAC_DIGEST_BYTES });
            outer.final_without_logical_reset(out);
        }
        // ---- Step 5: wipe secrets (best-effort) ----
        detail::secure_zero(key_pad, sizeof(key_pad));
        detail::secure_zero(i_pad, sizeof(i_pad));
        detail::secure_zero(o_pad, sizeof(o_pad));
        detail::secure_zero(inner_hash, sizeof(inner_hash));
    } // hmac

    void sha256::update(byte_view data) noexcept {
        if (!data) return; // consider this is no-op
        static_assert(sizeof(_data) == BLOCK_BYTES, "sha256: _data mismatch size");
        for (usize i = 0; i < data.size(); ++i) {
            _data[_data_count++] = data[i];
            if (_data_count == BLOCK_BYTES) {
                transform(byte_view{ _data, sizeof(_data) });
                _bit_length += 512;
                _data_count = 0;
            }
        }
    } // update

    //template<a::usize N>
    //void sha256::update(const u8(&block)[N]) noexcept {
    //    static_assert(sizeof(_data) == BLOCK_BYTES, "sha256: _data mismatch size");
    //    for (usize i = 0; i < N; ++i) {
    //        _data[_data_count++] = block[i];
    //        // if data block is full, transform it, reset the data length, and continue
    //        if (_data_count == 64) {
    //            transform(byte_view{ _data, sizeof(_data) });
    //            _bit_length += 512;
    //            _data_count = 0;
    //        }
    //    }
    //} // update

    // SHA-256 constants
    static A_CONSTEXPR_VAR a::u32 SHA256_K[64]{
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    void sha256::transform(byte_view buf) noexcept {
        u32 a, b, c, d, e, f, g, h;
        u32 t1, t2;
        u32 i, j;
        u32 m[64]; // fully initialized later
        i=j=0;
        for (; i<16; ++i, j += 4)
            m[i] = (u32(buf[j+0]) << 24) |
                   (u32(buf[j+1]) << 16) |
                   (u32(buf[j+2]) << 8 ) |
                   (u32(buf[j+3]) << 0 );

        for (; i<64; ++i)
            m[i] = sig1( m[i-2]  ) + m[i-7] +
                   sig0( m[i-15] ) + m[i-16];

        a=_state[0]; b=_state[1]; c=_state[2]; d=_state[3];
        e=_state[4]; f=_state[5]; g=_state[6]; h=_state[7];

        // Main loop
        for (i=0; i<64; ++i) {
            t1 = h + ep1(e) + ch(e,f,g) + SHA256_K[i] + m[i];
            t2 = ep0(a) + major(a,b,c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        _state[0]+=a; _state[1]+=b; _state[2]+=c; _state[3]+=d;
        _state[4]+=e; _state[5]+=f; _state[6]+=g; _state[7]+=h;
    } // transform

    void sha256::final_without_logical_reset(byte_view_mut out) noexcept {
        static_assert(sizeof(_data) == BLOCK_BYTES, "sha256: _data mismatch size");
        usize i = _data_count;

        if (_data_count < 56) {
            _data[i++] = 0x80;
            while (i < 56) _data[i++] = 0;
        }
        else {
            _data[i++] = 0x80;
            while (i < 64) _data[i++] = 0;
            transform(byte_view{ _data, sizeof(_data) });

            // FIX: prepare the second block = 56 zero bytes + 8 bytes length
            for (usize k = 0; k < 56; ++k) _data[k] = 0;
        }
        _bit_length += static_cast<a::u64>(_data_count) * 8;

        _data[56] = static_cast<u8>(_bit_length >> 56);
        _data[57] = static_cast<u8>(_bit_length >> 48);
        _data[58] = static_cast<u8>(_bit_length >> 40);
        _data[59] = static_cast<u8>(_bit_length >> 32);
        _data[60] = static_cast<u8>(_bit_length >> 24);
        _data[61] = static_cast<u8>(_bit_length >> 16);
        _data[62] = static_cast<u8>(_bit_length >> 8);
        _data[63] = static_cast<u8>(_bit_length >> 0);
        transform(byte_view{ _data, sizeof(_data) });

        for (i=0; i<4; ++i) {
            for (usize j=0; j<8; ++j) {
                out[i + 4*j] = (_state[j] >> (24 - 8*i)) & 0x000000ff;
            }
        }
    } // final_without_logical_reset

    template<a::usize N>
    void sha256::final_without_logical_reset(byte_view_mut_n<N> mutable_buffer) noexcept {
        static_assert(N >= DIGEST_BYTES, "sha256: out must be >= 32 bytes");
        final_without_logical_reset((byte_view_mut)mutable_buffer.v);
    }
    template<a::usize N>
    void sha256::final_without_logical_reset(u8(&mutable_buffer)[N]) noexcept {
        static_assert(N >= DIGEST_BYTES, "sha256: out must be >= 32 bytes");
        final_without_logical_reset(byte_view_mut{ mutable_buffer, N });
    }

    template<a::usize N>
    void sha256::final(byte_view_mut_n<N> mutable_buffer) noexcept {
        static_assert(N >= DIGEST_BYTES, "sha256: out must be >= 32 bytes");
        final_without_logical_reset((byte_view_mut)mutable_buffer.v);
        reset();
    }
    template<a::usize N>
    void sha256::final(u8(&mutable_buffer)[N]) noexcept {
        static_assert(N >= DIGEST_BYTES, "sha256: out must be >= 32 bytes");
        final_without_logical_reset(byte_view_mut{ mutable_buffer, N });
        reset();
    }
} // namespace crypto
} // namespace a