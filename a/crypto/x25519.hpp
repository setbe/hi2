// RFC 7748: X25519 (Curve25519 ECDH)
// Notes:
// - Scalar clamping per RFC 7748.
// - Constant-time Montgomery ladder with fe(5x51-bit limbs).
// - Public API is size-safe: only accepts 32-byte compile-time views / arrays.
// - shared_secret() returns false if the result is all-zero (recommended safety check).

#pragma once
#include "../a.hpp"
#include "detail/ct.hpp"
#include "detail/load_store.hpp"
#include "detail/wipe.hpp"

namespace a {
namespace crypto {

    struct x25519 {
        using u8 = a::u8;
        using u32 = a::u32;
        using u64 = a::u64;
        using u128 = a::u128;
        using usize = a::usize;

        using byte_view = a::byte_view;
        using byte_view_mut = a::byte_view_mut;
        template<usize N> using byte_view_n = a::byte_view_n<N>;
        template<usize N> using byte_view_mut_n = a::byte_view_mut_n<N>;

        // Constants
        static A_CONSTEXPR_VAR usize SCALAR_BYTES = 32;
        static A_CONSTEXPR_VAR usize PUBKEY_BYTES = 32;
        static A_CONSTEXPR_VAR usize SHARED_BYTES = 32;

        static_assert(SCALAR_BYTES == 32, "x25519: scalar is 32 bytes");
        static_assert(PUBKEY_BYTES == 32, "x25519: pubkey is 32 bytes");
        static_assert(SHARED_BYTES == 32, "x25519: shared secret is 32 bytes");

    private:
        // ------------------------------ fe (5x51) ------------------------------
        struct fe {
            u64 v[5]; // each limb < 2^51
        };

        static A_CONSTEXPR void clamp_scalar(u8 s[32]) noexcept {
            s[0] &= 248u;
            s[31] &= 127u;
            s[31] |= 64u;
        }

        static A_CONSTEXPR void fe_0(fe& x) noexcept { x.v[0] = x.v[1] = x.v[2] = x.v[3] = x.v[4] = 0; }
        static A_CONSTEXPR void fe_1(fe& x) noexcept { fe_0(x); x.v[0] = 1; }

        static A_CONSTEXPR void fe_copy(fe& d, const fe& a) noexcept {
            d.v[0] = a.v[0]; d.v[1] = a.v[1]; d.v[2] = a.v[2]; d.v[3] = a.v[3]; d.v[4] = a.v[4];
        }

        // (a+b) mod p in lazy form
        static A_CONSTEXPR void fe_add(fe& out, const fe& a, const fe& b) noexcept {
            out.v[0] = a.v[0] + b.v[0];
            out.v[1] = a.v[1] + b.v[1];
            out.v[2] = a.v[2] + b.v[2];
            out.v[3] = a.v[3] + b.v[3];
            out.v[4] = a.v[4] + b.v[4];
        }

        // (a-b) mod p in lazy form (add bias to avoid underflow)
        static A_CONSTEXPR void fe_sub(fe& out, const fe& a, const fe& b) noexcept {
            // bias: 2*p in limb form (sufficiently large)
            // p = 2^255 - 19, in 51-bit limbs: [2^51-19, 2^51-1, 2^51-1, 2^51-1, 2^51-1]
            // 2p: [2^52-38, 2^52-2, 2^52-2, 2^52-2, 2^52-2]
            const u64 b0 = ((u64)1 << 52) - 38;
            const u64 bi = ((u64)1 << 52) - 2;
            out.v[0] = a.v[0] + b0 - b.v[0];
            out.v[1] = a.v[1] + bi - b.v[1];
            out.v[2] = a.v[2] + bi - b.v[2];
            out.v[3] = a.v[3] + bi - b.v[3];
            out.v[4] = a.v[4] + bi - b.v[4];
        }

        static A_CONSTEXPR void fe_carry(fe& x) noexcept {
            const u64 mask = (((u64)1 << 51) - 1);

            // pass 1
            u64 c0 = x.v[0] >> 51; x.v[0] &= mask; x.v[1] += c0;
            u64 c1 = x.v[1] >> 51; x.v[1] &= mask; x.v[2] += c1;
            u64 c2 = x.v[2] >> 51; x.v[2] &= mask; x.v[3] += c2;
            u64 c3 = x.v[3] >> 51; x.v[3] &= mask; x.v[4] += c3;
            u64 c4 = x.v[4] >> 51; x.v[4] &= mask;

            // fold
            x.v[0] += c4 * 19u;

            // pass 2 (FULL)
            c0 = x.v[0] >> 51; x.v[0] &= mask; x.v[1] += c0;
            c1 = x.v[1] >> 51; x.v[1] &= mask; x.v[2] += c1;
            c2 = x.v[2] >> 51; x.v[2] &= mask; x.v[3] += c2;
            c3 = x.v[3] >> 51; x.v[3] &= mask; x.v[4] += c3;
            c4 = x.v[4] >> 51; x.v[4] &= mask;

            // fold again (optional but safe)
            x.v[0] += c4 * 19u;
            c0 = x.v[0] >> 51; x.v[0] &= mask; x.v[1] += c0;
        }


        // --- 64/128-bit helpers ---

        // 19*x = (16+2+1)*x
        A_NODISCARD static inline u64 mul19_u64(u64 x) noexcept { return (x<<4) + (x<<1) + x; }
        // floor(a / 2^51)
        A_NODISCARD static inline u64 shr_u128_51(u128 a) noexcept { return (a.lo>>51) | (a.hi << (64-51)); }
        A_NODISCARD static inline u64 and_mask51(u128 a) noexcept { return a.lo & ((1ull << 51) - 1ull); }
        A_NODISCARD static inline u128 madd(u128 acc, u64 x, u64 y) noexcept { return a::add_u128(acc, a::mul_u64(x, y)); };

        // out = a * b  (schoolbook + reduction with 19)
        static void fe_mul(fe& out, const fe& a, const fe& b) noexcept {
            const u64 a0 = a.v[0], a1 = a.v[1], a2 = a.v[2], a3 = a.v[3], a4 = a.v[4];
            const u64 b0 = b.v[0], b1 = b.v[1], b2 = b.v[2], b3 = b.v[3], b4 = b.v[4];

            const u64 b1_19 = mul19_u64(b1);
            const u64 b2_19 = mul19_u64(b2);
            const u64 b3_19 = mul19_u64(b3);
            const u64 b4_19 = mul19_u64(b4);

            u128 t0{ 0,0 }, t1{ 0,0 }, t2{ 0,0 }, t3{ 0,0 }, t4{ 0,0 };

            t0 = madd(t0, a0, b0);    t0 = madd(t0, a1, b4_19); t0 = madd(t0, a2, b3_19); t0 = madd(t0, a3, b2_19); t0 = madd(t0, a4, b1_19);
            t1 = madd(t1, a0, b1);    t1 = madd(t1, a1, b0);    t1 = madd(t1, a2, b4_19); t1 = madd(t1, a3, b3_19); t1 = madd(t1, a4, b2_19);
            t2 = madd(t2, a0, b2);    t2 = madd(t2, a1, b1);    t2 = madd(t2, a2, b0);    t2 = madd(t2, a3, b4_19); t2 = madd(t2, a4, b3_19);
            t3 = madd(t3, a0, b3);    t3 = madd(t3, a1, b2);    t3 = madd(t3, a2, b1);    t3 = madd(t3, a3, b0);    t3 = madd(t3, a4, b4_19);
            t4 = madd(t4, a0, b4);    t4 = madd(t4, a1, b3);    t4 = madd(t4, a2, b2);    t4 = madd(t4, a3, b1);    t4 = madd(t4, a4, b0);

            const u64 mask51 = (1ull << 51) - 1ull;

            u64 r0 = and_mask51(t0); u64 c = shr_u128_51(t0);    t1 = add_u128_u64(t1, c);
            u64 r1 = and_mask51(t1);     c = shr_u128_51(t1);    t2 = add_u128_u64(t2, c);
            u64 r2 = and_mask51(t2);     c = shr_u128_51(t2);    t3 = add_u128_u64(t3, c);
            u64 r3 = and_mask51(t3);     c = shr_u128_51(t3);    t4 = add_u128_u64(t4, c);
            u64 r4 = and_mask51(t4);     c = shr_u128_51(t4);

            // fold carry: r0 += c*19
            u64 r0_add = mul19_u64(c);
            r0 += r0_add;
            u64 carry0 = (r0 >> 51);
            r0 &= mask51;
            r1 += carry0;

            out.v[0] = r0; out.v[1] = r1; out.v[2] = r2; out.v[3] = r3; out.v[4] = r4;
            fe_carry(out);
        }

        static void fe_sqr(fe& out, const fe& a) noexcept { fe_mul(out, a, a); }

        // constant-time conditional swap (swap=0/1)
        static A_CONSTEXPR void fe_cswap(fe& a, fe& b, u64 swap) noexcept {
            // mask = 0xffff.. if swap==1 else 0
            const u64 mask = (u64)0 - (swap & 1u);
            for (int i = 0; i < 5; ++i) {
                const u64 t = mask & (a.v[i] ^ b.v[i]);
                a.v[i] ^= t;
                b.v[i] ^= t;
            }
        }

        // load u-coordinate (32 bytes little-endian), clear top bit
        static void fe_from_u(fe& out, const u8 in[32]) noexcept {
            u8 t[32];
            for (usize i = 0; i < 32; ++i) t[i] = in[i];
            t[31] &= 0x7Fu;

            const u64 x0 = detail::load64_le(t + 0);
            const u64 x1 = detail::load64_le(t + 8);
            const u64 x2 = detail::load64_le(t + 16);
            const u64 x3 = detail::load64_le(t + 24);

            // Split 255 bits into 5 limbs of 51 bits (ref10 style)
            out.v[0] = x0 & (((u64)1 << 51) - 1);
            out.v[1] = ((x0 >> 51) | (x1 << 13)) & (((u64)1 << 51) - 1);
            out.v[2] = ((x1 >> 38) | (x2 << 26)) & (((u64)1 << 51) - 1);
            out.v[3] = ((x2 >> 25) | (x3 << 39)) & (((u64)1 << 51) - 1);
            out.v[4] = (x3 >> 12) & (((u64)1 << 51) - 1);
        }

        static A_CONSTEXPR u64 ct_ge(u64 a, u64 b) noexcept {
            // returns 1 if a>=b else 0 (constant-time-ish for unsigned)
            return 1u ^ ((a-b) >> 63);
        }

        // encode canonical field element to 32 bytes
        static void fe_to_u(u8 out[32], fe x) noexcept {
            // fully carry
            fe_carry(x); fe_carry(x);

            // conditional subtract p
            // p limbs: [2^51-19, 2^51-1, 2^51-1, 2^51-1, 2^51-1]
            const u64 p0 = (((u64)1 << 51) - 19);
            const u64 pi = (((u64)1 << 51) - 1);
            
            // compute x - p into t (with borrow)
            u64 t0 = x.v[0] - p0;
            u64 t1 = x.v[1] - pi - (t0 >> 63);
            u64 t2 = x.v[2] - pi - (t1 >> 63);
            u64 t3 = x.v[3] - pi - (t2 >> 63);
            u64 t4 = x.v[4] - pi - (t3 >> 63);
            
            // mask = 0xffff.. if no borrow (t4 >= 0), else 0
            const u64 mask = (u64)0 - (1u ^ (u64)(t4 >> 63));
            // select: if mask==all1 => use t, else use x
            x.v[0] = (x.v[0] & ~mask) | (t0 & mask);
            x.v[1] = (x.v[1] & ~mask) | (t1 & mask);
            x.v[2] = (x.v[2] & ~mask) | (t2 & mask);
            x.v[3] = (x.v[3] & ~mask) | (t3 & mask);
            x.v[4] = (x.v[4] & ~mask) | (t4 & mask);
            
            // pack limbs into 4x64
            const u64 x0 = x.v[0] | (x.v[1] << 51);
            const u64 x1 = (x.v[1] >> 13) | (x.v[2] << 38);
            const u64 x2 = (x.v[2] >> 26) | (x.v[3] << 25);
            const u64 x3 = (x.v[3] >> 39) | (x.v[4] << 12);
            detail::store64_le(out + 0, x0);
            detail::store64_le(out + 8, x1);
            detail::store64_le(out + 16, x2);
            detail::store64_le(out + 24, x3);
        }

        static void fe_invert(fe& out, const fe& z) noexcept {
            fe t0, t1, t2, t3;

            fe_sqr(t0, z);          // z^2
            fe_sqr(t1, t0);         // z^4
            fe_sqr(t1, t1);         // z^8
            fe_mul(t1, z, t1);      // z^9
            fe_mul(t0, t0, t1);     // z^11          (KEEP t0 = z^11)
            fe_sqr(t2, t0);         // z^22
            fe_mul(t1, t1, t2);     // z^31 = z^(2^5-1)

            fe_sqr(t2, t1);
            for (int i = 0; i < 4; ++i) fe_sqr(t2, t2);
            fe_mul(t1, t2, t1);     // z^(2^10-1)

            fe_sqr(t2, t1);
            for (int i = 0; i < 9; ++i) fe_sqr(t2, t2);
            fe_mul(t2, t2, t1);     // z^(2^20-1)

            fe_sqr(t3, t2);
            for (int i = 0; i < 19; ++i) fe_sqr(t3, t3);
            fe_mul(t2, t3, t2);     // z^(2^40-1)

            fe_sqr(t2, t2);
            for (int i = 0; i < 9; ++i) fe_sqr(t2, t2);
            fe_mul(t1, t2, t1);     // z^(2^50-1)

            fe_sqr(t2, t1);
            for (int i = 0; i < 49; ++i) fe_sqr(t2, t2);
            fe_mul(t2, t2, t1);     // z^(2^100-1)

            fe_sqr(t3, t2);
            for (int i = 0; i < 99; ++i) fe_sqr(t3, t3);
            fe_mul(t2, t3, t2);     // z^(2^200-1)

            fe_sqr(t2, t2);
            for (int i = 0; i < 49; ++i) fe_sqr(t2, t2);
            fe_mul(t1, t2, t1);     // z^(2^250-1)

            // 5 squarings => z^(2^255 - 32)
            for (int i = 0; i < 5; ++i) fe_sqr(t1, t1);

            // multiply by z^11 => z^(2^255 - 21) = z^(p-2)
            fe_mul(out, t1, t0);
        }


        // Montgomery ladder: out = scalar * u
        static void scalar_mult_u(u8 out[32], const u8 scalar_in[32], const u8 u_in[32]) noexcept {
            u8 e[32];
            for (usize i = 0; i < 32; ++i) e[i] = scalar_in[i];
            clamp_scalar(e);

            fe x1; fe_from_u(x1, u_in);

            fe x2, z2, x3, z3;
            fe_1(x2);
            fe_0(z2);
            fe_copy(x3, x1);
            fe_1(z3);

            // A24 = 121666
            fe A24; fe_0(A24); A24.v[0] = 121666u;

            u64 swap = 0;

            for (int t = 254; t >= 0; --t) {
                const u64 k_t = (e[t >> 3] >> (t & 7)) & 1u;
                swap ^= k_t;
                fe_cswap(x2, x3, swap);
                fe_cswap(z2, z3, swap);
                swap = k_t;

                fe a, b, c, d, da, cb, aa, bb, e2, aa24, tmp;

                fe_add(a, x2, z2);
                fe_sub(b, x2, z2);
                fe_add(c, x3, z3);
                fe_sub(d, x3, z3);

                fe_mul(da, d, a);
                fe_mul(cb, c, b);

                fe_add(tmp, da, cb);
                fe_sqr(x3, tmp);

                fe_sub(tmp, da, cb);
                fe_sqr(tmp, tmp);
                fe_mul(z3, tmp, x1);

                fe_sqr(aa, a);
                fe_sqr(bb, b);
                fe_sub(e2, aa, bb);

                fe_mul(x2, aa, bb);

                fe_mul(aa24, e2, A24);
                fe_add(aa24, aa24, aa);
                fe_mul(z2, e2, aa24);
            }

            fe_cswap(x2, x3, swap);
            fe_cswap(z2, z3, swap);

            fe z2inv;
            fe_invert(z2inv, z2);

            fe x_aff;
            fe_mul(x_aff, x2, z2inv);

            fe_to_u(out, x_aff);

            detail::secure_zero(e, sizeof(e));
        }

        A_NODISCARD static A_CONSTEXPR bool is_all_zero32(const u8 x[32]) noexcept {
            u8 acc = 0;
            for (usize i = 0; i < 32; ++i) acc |= x[i];
            return acc == 0;
        }

    public:
        // --------------------------------------------------------------------
        //                              Public API
        // --------------------------------------------------------------------

        template<usize N>
        A_NODISCARD static A_CONSTEXPR bool ct_equal32(byte_view_n<N> a, byte_view_n<N> b) noexcept {
            static_assert(N == 32, "x25519::ct_equal32: size must be 32");
            return detail::ct_equal_n<32>(a, b);
        }

        // pub = priv * basepoint(9)
        static void generate_public(byte_view_n<SCALAR_BYTES> priv32,
                                    byte_view_mut_n<PUBKEY_BYTES> pub32) noexcept {
            u8 priv[32];
            for (usize i = 0; i < 32; ++i) priv[i] = priv32.v[i];

            u8 base[32]{};
            base[0] = 9;

            scalar_mult_u(pub32.v.data(), priv, base);

            detail::secure_zero(priv, sizeof(priv));
        }

        // out = priv * peer_pub; returns false if shared secret is all-zero
        A_NODISCARD static bool shared_secret(byte_view_n<SCALAR_BYTES> priv32,
                                               byte_view_n<PUBKEY_BYTES> peer_pub32,
                                               byte_view_mut_n<SHARED_BYTES> out32) noexcept {
            u8 priv[32];
            for (usize i = 0; i < 32; ++i) priv[i] = priv32.v[i];

            scalar_mult_u(out32.v.data(), priv, peer_pub32.v.data());

            const bool ok = !is_all_zero32(out32.v.data());
            detail::secure_zero(priv, sizeof(priv));
            return ok;
        }

        // overloads (C-arrays)
        static void generate_public(const u8(&priv)[32], u8(&pub)[32]) noexcept {
            generate_public(a::as_view(priv), a::as_view_mut(pub));
        }
        A_NODISCARD static bool shared_secret(const u8(&priv)[32], const u8(&peer_pub)[32], u8(&out)[32]) noexcept {
            return shared_secret(a::as_view(priv), a::as_view(peer_pub), a::as_view_mut(out));
        }
    };

} // namespace crypto
} // namespace a
