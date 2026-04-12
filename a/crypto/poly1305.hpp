// RFC 8439: ChaCha20-Poly1305 (Poly1305 MAC, 16-byte tag, one-time key r||s)
#pragma once

#include "../a.hpp"
#include "detail/ct.hpp"
#include "detail/load_store.hpp"
#include "detail/wipe.hpp"
#include "chacha20.hpp"

namespace a {
namespace crypto {
    struct poly1305 {
        friend struct aead_chacha20_poly1305;

        using u8 = a::u8;
        using u32 = a::u32;
        using u64 = a::u64;
        using usize = a::usize;

        using byte_view = a::byte_view;
        using byte_view_mut = a::byte_view_mut;
        template<usize N> using byte_view_n = a::byte_view_n<N>;
        template<usize N> using byte_view_mut_n = a::byte_view_mut_n<N>;

        // Constants
        static A_CONSTEXPR_VAR usize TAG_BYTES = 16;
        static A_CONSTEXPR_VAR usize OTKEY_BYTES = 32; // r(16) || s(16)

        static_assert(TAG_BYTES == 16, "poly1305: tag is 16 bytes");
        static_assert(OTKEY_BYTES == 32, "poly1305: one-time key is 32 bytes");

        // RAII
        explicit A_CONSTEXPR poly1305() noexcept
            : _h{ 0,0,0,0,0 }, _r{ 0,0,0,0,0 }, _r5{ 0,0,0,0,0 }, _s{ 0,0,0,0 },
            _buf{ 0 }, _buf_used{ 0 } {}
        ~poly1305() noexcept { wipe(); }

    private:
        u32 _h[5]; // h accumulator (5x26-bit limbs)
        u32 _r[5];  // r (clamped) (5x26-bit limbs)
        u32 _r5[5]; // r*5

        u8 __padding__[4]{};

        u8  _buf[16]; // partial block buffer
        u32 _buf_used;
        u32 _s[4]; // s pad (4x32-bit LE)

        A_CONSTEXPR static void clamp_r(u8 r[16]) noexcept {
            r[3] &= 15u;  r[7] &= 15u;  r[11] &= 15u; r[15] &= 15u;
            r[4] &= 252u; r[8] &= 252u; r[12] &= 252u;
        }

        void blocks(const u8* m, usize count16) noexcept {
            for (usize n = 0; n < count16; ++n, m += 16) {
                u32 t0 = (u32)m[0] | ((u32)m[1] << 8) | ((u32)m[2]<<16) | (((u32)m[3] & 0x3u) << 24);
                u32 t1 = ((u32)m[3] >> 2) | ((u32)m[4] << 6) | ((u32)m[5] << 14) | (((u32)m[6] & 0xFu) << 22);
                u32 t2 = ((u32)m[6] >> 4) | ((u32)m[7] << 4) | ((u32)m[8] << 12) | (((u32)m[9] & 0x3Fu) << 20);
                u32 t3 = ((u32)m[9] >> 6) | ((u32)m[10] << 2) | ((u32)m[11] << 10) | ((u32)m[12] << 18);
                u32 t4 = (u32)m[13] | ((u32)m[14] << 8) | ((u32)m[15] << 16);

                t4 += (1u << 24); // implicit 1 for full block

                u32 h0 = (u64)_h[0]+t0;
                u32 h1 = (u64)_h[1]+t1;
                u32 h2 = (u64)_h[2]+t2;
                u32 h3 = (u64)_h[3]+t3;
                u32 h4 = (u64)_h[4]+t4;

                u64 d0 = a::mul_u32(h0, _r[0]) + a::mul_u32(h1, _r5[4]) + a::mul_u32(h2, _r5[3]) + a::mul_u32(h3, _r5[2]) + a::mul_u32(h4, _r5[1]);
                u64 d1 = a::mul_u32(h0, _r[1]) + a::mul_u32(h1, _r[0])  + a::mul_u32(h2, _r5[4]) + a::mul_u32(h3, _r5[3]) + a::mul_u32(h4, _r5[2]);
                u64 d2 = a::mul_u32(h0, _r[2]) + a::mul_u32(h1, _r[1])  + a::mul_u32(h2, _r[0])  + a::mul_u32(h3, _r5[4]) + a::mul_u32(h4, _r5[3]);
                u64 d3 = a::mul_u32(h0, _r[3]) + a::mul_u32(h1, _r[2])  + a::mul_u32(h2, _r[1])  + a::mul_u32(h3, _r[0])  + a::mul_u32(h4, _r5[4]);
                u64 d4 = a::mul_u32(h0, _r[4]) + a::mul_u32(h1, _r[3])  + a::mul_u32(h2, _r[2])  + a::mul_u32(h3, _r[1])  + a::mul_u32(h4, _r[0]);

                u64 c;
                         c = (d0 >> 26); _h[0] = (u32)(d0 & 0x3ffffffu);
                d1 += c; c = (d1 >> 26); _h[1] = (u32)(d1 & 0x3ffffffu);
                d2 += c; c = (d2 >> 26); _h[2] = (u32)(d2 & 0x3ffffffu);
                d3 += c; c = (d3 >> 26); _h[3] = (u32)(d3 & 0x3ffffffu);
                d4 += c;                 _h[4] = (u32)(d4 & 0x3ffffffu);

                _h[0] += (u32)((d4 >> 26) * 5u);
                _h[1] += (_h[0] >> 26);
                _h[0] &= 0x3ffffffu;
            }
        }
        void block_partial(const u8* m, usize len) noexcept {
            for (usize i = 0; i < len; ++i) _buf[i] = m[i];
            for (usize i = len; i < 16; ++i) _buf[i] = 0;

            const u8* b = _buf;

            u32 t0 = (u32)b[0] | ((u32)b[1] << 8)  | ((u32)b[2] << 16) | (((u32)b[3] & 0x3u) << 24);
            u32 t1 = ((u32)b[3] >> 2) | ((u32)b[4] << 6) | ((u32)b[5] << 14) | (((u32)b[6] & 0xFu) << 22);
            u32 t2 = ((u32)b[6] >> 4) | ((u32)b[7] << 4) | ((u32)b[8] << 12) | (((u32)b[9] & 0x3Fu) << 20);
            u32 t3 = ((u32)b[9] >> 6) | ((u32)b[10] << 2) | ((u32)b[11] << 10) | ((u32)b[12] << 18);
            u32 t4 = (u32)b[13] | ((u32)b[14] << 8) | ((u32)b[15] << 16);

            // implicit 1 at bit position 8*len
            const usize bit = len * 8;
            const usize limb = bit / 26;
            const usize off = bit % 26;
            const u32 add = (u32)(1u << off);

            if (limb == 0) t0 += add;
            else if (limb == 1) t1 += add;
            else if (limb == 2) t2 += add;
            else if (limb == 3) t3 += add;
            else                t4 += add;

            u32 h0 = (u64)_h[0] + t0;
            u32 h1 = (u64)_h[1] + t1;
            u32 h2 = (u64)_h[2] + t2;
            u32 h3 = (u64)_h[3] + t3;
            u32 h4 = (u64)_h[4] + t4;
            
            u64 d0 = a::mul_u32(h0, _r[0]) + a::mul_u32(h1, _r5[4]) + a::mul_u32(h2, _r5[3]) + a::mul_u32(h3, _r5[2]) + a::mul_u32(h4, _r5[1]);
            u64 d1 = a::mul_u32(h0, _r[1]) + a::mul_u32(h1, _r[0])  + a::mul_u32(h2, _r5[4]) + a::mul_u32(h3, _r5[3]) + a::mul_u32(h4, _r5[2]);
            u64 d2 = a::mul_u32(h0, _r[2]) + a::mul_u32(h1, _r[1])  + a::mul_u32(h2, _r[0])  + a::mul_u32(h3, _r5[4]) + a::mul_u32(h4, _r5[3]);
            u64 d3 = a::mul_u32(h0, _r[3]) + a::mul_u32(h1, _r[2])  + a::mul_u32(h2, _r[1])  + a::mul_u32(h3, _r[0])  + a::mul_u32(h4, _r5[4]);
            u64 d4 = a::mul_u32(h0, _r[4]) + a::mul_u32(h1, _r[3])  + a::mul_u32(h2, _r[2])  + a::mul_u32(h3, _r[1])  + a::mul_u32(h4, _r[0]);

            u64 c;
                     c = (d0 >> 26); _h[0] = (u32)(d0 & 0x3ffffffu);
            d1 += c; c = (d1 >> 26); _h[1] = (u32)(d1 & 0x3ffffffu);
            d2 += c; c = (d2 >> 26); _h[2] = (u32)(d2 & 0x3ffffffu);
            d3 += c; c = (d3 >> 26); _h[3] = (u32)(d3 & 0x3ffffffu);
            d4 += c;                 _h[4] = (u32)(d4 & 0x3ffffffu);

            _h[0] += (u32)((d4 >> 26) * 5u);
            _h[1] += (_h[0] >> 26);
            _h[0] &= 0x3ffffffu;
        }
        void finish(u8 tag[TAG_BYTES]) const noexcept {
            u32 h0 = _h[0], h1 = _h[1], h2 = _h[2], h3 = _h[3], h4 = _h[4];

            u32 c;

            c = (h1 >> 26); h1 &= 0x3ffffffu; h2 += c;
            c = (h2 >> 26); h2 &= 0x3ffffffu; h3 += c;
            c = (h3 >> 26); h3 &= 0x3ffffffu; h4 += c;
            c = (h4 >> 26); h4 &= 0x3ffffffu; h0 += c * 5u;
            c = (h0 >> 26); h0 &= 0x3ffffffu; h1 += c;

            // g = h + 5, then check carry into 2^130 (>= p)
            u32 g0 = h0 + 5u;
            u32 g1 = h1 + (g0 >> 26); g0 &= 0x3ffffffu;
            u32 g2 = h2 + (g1 >> 26); g1 &= 0x3ffffffu;
            u32 g3 = h3 + (g2 >> 26); g2 &= 0x3ffffffu;
            u32 g4 = h4 + (g3 >> 26) - (1u << 26); g3 &= 0x3ffffffu;

            const u32 mask = (g4 >> 31) - 1u; // all 1s if g4 >= 0 else 0
            h0 = (h0 & ~mask) | (g0 & mask);
            h1 = (h1 & ~mask) | (g1 & mask);
            h2 = (h2 & ~mask) | (g2 & mask);
            h3 = (h3 & ~mask) | (g3 & mask);
            h4 = (h4 & ~mask) | ((g4 + (1u << 26)) & mask);

            // collapse to 128-bit little-endian (4x32)
            u64 f0 = ((u64)h0 | ((u64)h1 << 26)) & 0xffffffffu;
            u64 f1 = (((u64)h1 >> 6) | ((u64)h2 << 20)) & 0xffffffffu;
            u64 f2 = (((u64)h2 >> 12) | ((u64)h3 << 14)) & 0xffffffffu;
            u64 f3 = (((u64)h3 >> 18) | ((u64)h4 << 8)) & 0xffffffffu;

            f0 += _s[0];
            f1 += _s[1] + (f0 >> 32);
            f2 += _s[2] + (f1 >> 32);
            f3 += _s[3] + (f2 >> 32);

            detail::store32_le(tag + 0, (u32)f0);
            detail::store32_le(tag + 4, (u32)f1);
            detail::store32_le(tag + 8, (u32)f2);
            detail::store32_le(tag + 12, (u32)f3);
        }

        void init(byte_view otk32) noexcept {
            if (!otk32 || otk32.size() != OTKEY_BYTES) return;

            u8 rbytes[16];
            for (usize i = 0; i < 16; ++i) rbytes[i] = otk32[i];
            clamp_r(rbytes);

            u32 t0 = detail::load32_le(rbytes + 0);
            u32 t1 = detail::load32_le(rbytes + 4);
            u32 t2 = detail::load32_le(rbytes + 8);
            u32 t3 = detail::load32_le(rbytes + 12);

            _r[0] = (t0) & 0x3ffffffu;
            _r[1] = ((t0 >> 26) | (t1 << 6)) & 0x3ffffffu;
            _r[2] = ((t1 >> 20) | (t2 << 12)) & 0x3ffffffu;
            _r[3] = ((t2 >> 14) | (t3 << 18)) & 0x3ffffffu;
            _r[4] = (t3 >> 8);

            for (usize i = 0; i < 5; ++i) _r5[i] = _r[i] * 5u;

            _s[0] = detail::load32_le(otk32.data() + 16);
            _s[1] = detail::load32_le(otk32.data() + 20);
            _s[2] = detail::load32_le(otk32.data() + 24);
            _s[3] = detail::load32_le(otk32.data() + 28);

            reset();
        }
        void update(byte_view m) noexcept {
            if (!m) return;

            usize off = 0;
            usize len = m.size();

            if (_buf_used != 0) {
                usize need = 16u - _buf_used;
                if (need > len) need = len;
                for (usize i=0; i<need; ++i) _buf[_buf_used + i] = m[off + i];
                _buf_used += (u32)need;
                off += need;
                len -= need;
                if (_buf_used == 16u) {
                    blocks(_buf, 1);
                    _buf_used = 0;
                }
            }

            if (len >= 16u) {
                usize full = len / 16u;
                blocks(m.data() + off, full);
                usize consumed = full * 16u;
                off += consumed;
                len -= consumed;
            }

            for (usize i=0; i<len; ++i) _buf[i] = m[off + i];
            _buf_used = (u32)len;
        }
        void final(byte_view_mut out_tag16) noexcept {
            if (!out_tag16 || out_tag16.size() < TAG_BYTES) return;

            if (_buf_used != 0) {
                block_partial(_buf, _buf_used);
                _buf_used = 0;
            }

            u8 tag[TAG_BYTES];
            finish(tag);

            for (usize i = 0; i < TAG_BYTES; ++i) out_tag16[i] = tag[i];

            // one-time key MAC: scrub everything after use
            wipe();
        }

    public:
        // --------------------------------------------------------------------
        //                              Public API
        // --------------------------------------------------------------------

        void reset() noexcept {
            for (usize i = 0; i < 5; ++i) _h[i] = 0;
            for (usize i = 0; i < 16; ++i) _buf[i] = 0;
            _buf_used = 0;
        }

        void wipe() noexcept {
            detail::secure_zero(_h, sizeof(_h));
            detail::secure_zero(_r, sizeof(_r));
            detail::secure_zero(_r5, sizeof(_r5));
            detail::secure_zero(_s, sizeof(_s));
            detail::secure_zero(_buf, sizeof(_buf));
            detail::secure_zero(&_buf_used, sizeof(_buf_used));
            _buf_used = 0;
        }

        // init with one-time key (r||s)
        template<usize N> void init(byte_view_n<N> otk) noexcept {
            static_assert(N == OTKEY_BYTES, "poly1305::init: one-time key must be 32 bytes");
            init((byte_view)otk.v);
        }
        template<usize N> void init(const u8(&otk)[N]) noexcept { init(a::as_view(otk)); }

        template<usize N> void update(a::byte_view_n<N> m) noexcept { update((a::byte_view)m.v); }
        template<usize N> void update(const u8(&m)[N]) noexcept { update(a::as_view(m)); }
        
        template<usize N> void final(byte_view_mut_n<N> out_tag16) noexcept {
            static_assert(N >= TAG_BYTES, "poly1305::final: out must be >= 16 bytes");
            final((byte_view_mut)out_tag16.v);
        }
        template<usize N> void final(u8(&out)[N]) noexcept {
            static_assert(N >= TAG_BYTES, "poly1305::final: out must be >= 16 bytes");
            final(byte_view_mut{ out, N });
        }

        template<usize N>
        A_NODISCARD static A_CONSTEXPR bool ct_equal(byte_view_n<N> a, byte_view_n<N> b) noexcept {
            return detail::ct_equal_n<N>(a, b);
        }    
        template<usize NK, usize NN>
        static void derive_otk_from_chacha20(byte_view_n<NK> key,
                                             byte_view_n<NN> nonce,
                                             byte_view_mut_n<OTKEY_BYTES> out_otk) noexcept {
            static_assert(NK == a::crypto::chacha20::KEY_BYTES, "key must be 32 bytes");
            static_assert(NN == a::crypto::chacha20::NONCE_BYTES, "nonce must be 12 bytes");
            a::crypto::chacha20 c;
            c.init(key, nonce, 0); // counter=0
            c.keystream(out_otk);
        }

        template<usize NK, usize NN>
        static void derive_otk_from_chacha20(const u8(&key)[NK],
                                             const u8(&nonce)[NN],
                                             u8(&out_otk)[OTKEY_BYTES]) noexcept {
            derive_otk_from_chacha20(a::as_view(key), a::as_view(nonce), a::as_view_mut(out_otk));
        }

        template<usize NK, usize NN, usize NM>
        static void mac_chacha20(byte_view_n<NK> key,
                                 byte_view_n<NN> nonce,
                                 byte_view_n<NM> msg,
                                 byte_view_mut_n<TAG_BYTES> out_tag) noexcept {
            u8 otk[OTKEY_BYTES]{};
            derive_otk_from_chacha20(key, nonce, a::as_view_mut(otk));
            poly1305 p;
            p.init(a::as_view(otk));
            p.update(msg);
            p.final(out_tag);
        }
        template<usize NM>
        static void mac_chacha20(const u8(&key)[32], const u8(&nonce)[12],
                                 const u8(&msg)[NM], u8(&tag)[TAG_BYTES]) noexcept {
            mac_chacha20(a::as_view(key), a::as_view(nonce), a::as_view(msg), a::as_view_mut(tag));
        }

        
        template<usize NK, usize NN, usize NM>
        A_NODISCARD static bool verify_chacha20(byte_view_n<NK> key,
                                                byte_view_n<NN> nonce,
                                                byte_view_n<NM> msg,
                                                byte_view_n<TAG_BYTES> tag) noexcept {
            u8 got[TAG_BYTES]{};
            mac_chacha20(key, nonce, msg, a::as_view_mut(got));
            return ct_equal<TAG_BYTES>(a::as_view(got), tag);
        }

        template<usize NM>
        A_NODISCARD static bool verify(byte_view_n<OTKEY_BYTES> otk,
                                        byte_view_n<NM> msg,
                                        byte_view_n<TAG_BYTES> tag) noexcept {
            u8 got[TAG_BYTES]{};
            poly1305 p;
            p.init(otk);
            p.update(msg);
            p.final(got);
            return ct_equal<TAG_BYTES>(a::as_view(got), tag);
        }

        template<usize NM>
        A_NODISCARD static bool verify(const u8(&otk)[OTKEY_BYTES],
                                        const u8(&msg)[NM],
                                        u8(&tag)[TAG_BYTES]) noexcept {
            u8 got[TAG_BYTES]{};
            poly1305 p;
            p.init(otk);
            p.update(msg);
            p.final(got);
            return ct_equal<TAG_BYTES>(a::as_view(got), tag);
        }
    }; // struct poly1305
} // namespace crypto
} // namespace a
