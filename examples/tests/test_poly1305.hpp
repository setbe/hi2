#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/crypto/poly1305.hpp"

// Tests cover:
// - Poly1305 core (RFC 8439 / RFC 7539 test vector)
// - ChaCha20-derived one-time key + Poly1305 (RFC 8439 AEAD subcomponents)
// - verify / ct_equal APIs (size-safe)
//
// Tests use public API:
// - cl::poly1305::verify(otk32, msg, tag16)
// - cl::poly1305::derive_otk_from_chacha20(key, nonce, out_otk32)
// - cl::poly1305::mac_chacha20(key, nonce, msg, out_tag16)
// - cl::poly1305::verify_chacha20(key, nonce, msg, tag16)
// - cl::poly1305::ct_equal(a, b)

namespace test_poly1305 {
    using u8 = io::u8;
    using usize = io::usize;

    static inline u8 hex_nibble(char c) noexcept {
        if (c >= '0' && c <= '9') return (u8)(c - '0');
        if (c >= 'a' && c <= 'f') return (u8)(10 + (c - 'a'));
        if (c >= 'A' && c <= 'F') return (u8)(10 + (c - 'A'));
        return 0xFF;
    }

    template<usize N>
    static inline void hex_to_bytes(const char(&hex)[N], u8(&out)[(N - 1) / 2]) noexcept {
        static_assert(N >= 1, "hex literal required");
        static_assert(((N - 1) % 2) == 0, "hex string must have even length");
        constexpr usize OUT_N = (N - 1) / 2;

        for (usize i = 0; i < OUT_N; ++i) {
            const u8 hi = hex_nibble(hex[i * 2 + 0]);
            const u8 lo = hex_nibble(hex[i * 2 + 1]);
            out[i] = (u8)((hi << 4) | lo);
        }
    }

    template<usize N>
    static inline void require_eq_bytes(const u8(&got)[N], const u8(&exp)[N]) {
        for (usize i = 0; i < N; ++i) REQUIRE(got[i] == exp[i]);
    }

    // -------------------------------------------------------------------------
    // RFC test vectors
    // -------------------------------------------------------------------------

    // RFC 8439 (and RFC 7539) Poly1305 test vector:
    //   key  = 85d6be78...f51b (32 bytes)
    //   msg  = "Cryptographic Forum Research Group"
    //   tag  = a8061dc1305136c6c22b8baf0c0127a9
    TEST_CASE("Poly1305: core test vector (RFC 8439)", "[poly1305][rfc8439][core]") {
        u8 otk[32]{};
        hex_to_bytes(
            "85d6be7857556d337f4452fe42d506a8"
            "0103808afb0db2fd4abff6af4149f51b",
            otk
        );

#ifdef IO_CXX_17
        constexpr io::char_view msg{ "Cryptographic Forum Research Group" };
        constexpr usize MSG_N = msg.size();
#else
        const char* msg = "Cryptographic Forum Research Group";
        constexpr usize MSG_N = 34;
#endif

        u8 m[MSG_N]{};
        for (usize i = 0; i < MSG_N; ++i) m[i] = (u8)msg[i];

        cl::poly1305 p;
        p.init(otk);
        p.update(m);

        u8 tag[16]{};
        p.final(tag);

        u8 exp[16]{};
        hex_to_bytes("a8061dc1305136c6c22b8baf0c0127a9", exp);

        require_eq_bytes(tag, exp);

        // verify() should pass
        REQUIRE(cl::poly1305::verify(otk, m, exp));

        // verify() should fail on 1-bit change
        exp[0] ^= 1;
        REQUIRE_FALSE(cl::poly1305::verify(otk, m, exp));
    }

    // RFC 8439 2.6.2: Poly1305 key generation using ChaCha20
    // key = 80..9f, nonce = 070000004041424344454647, counter=0
    // otk = 7bac2b25...  (32 bytes)
    TEST_CASE("Poly1305: derive_otk_from_chacha20 (RFC 8439)", "[poly1305][rfc8439][otk]") {
        u8 key[32]{};
        for (usize i=0; i<32; ++i) key[i] = (u8)(0x80u + i);

        u8 nonce[12]{};
        hex_to_bytes("070000004041424344454647", nonce);

        u8 otk[32]{};
        cl::poly1305::derive_otk_from_chacha20(key, nonce, otk);

        u8 exp[32]{};
        hex_to_bytes(
            "7bac2b252db447af09b67a55a4e95584"
            "0ae1d6731075d9eb2a9375783ed553ff",
            exp
        );

        require_eq_bytes(otk, exp);
    }

    // -------------------------------------------------------------------------
    // Consistency checks (NOT AEAD)
    // -------------------------------------------------------------------------

    TEST_CASE("Poly1305: mac_chacha20 equals derive_otk + poly1305", "[poly1305][mac][consistency]") {
        u8 key[32]{};
        u8 nonce[12]{};

        for (usize i = 0; i < 32; ++i) key[i] = (u8)(i * 3u + 1u);
        for (usize i = 0; i < 12; ++i) nonce[i] = (u8)(0xA0u + i);

        u8 msg[257]{};
        for (usize i = 0; i < 257; ++i) msg[i] = (u8)(i * 17u + 3u);

        u8 t1[16]{};
        cl::poly1305::mac_chacha20(io::as_view(key), io::as_view(nonce), io::as_view(msg), io::as_view_mut(t1));

        u8 otk[32]{};
        cl::poly1305::derive_otk_from_chacha20(io::as_view(key), io::as_view(nonce), io::as_view_mut(otk));

        u8 t2[16]{};
        {
            cl::poly1305 p;
            p.init(io::as_view(otk));
            p.update(io::as_view(msg));
            p.final(t2);
        }

        require_eq_bytes(t1, t2);

        REQUIRE(cl::poly1305::verify_chacha20(io::as_view(key), io::as_view(nonce), io::as_view(msg), io::as_view(t1)));

        t1[0] ^= 1;
        REQUIRE_FALSE(cl::poly1305::verify_chacha20(io::as_view(key), io::as_view(nonce), io::as_view(msg), io::as_view(t1)));
    }
    // -------------------------------------------------------------------------
    // Behavioral / invariants
    // -------------------------------------------------------------------------

    TEST_CASE("Poly1305: split update equals single update", "[poly1305][update]") {
        u8 otk[32]{};
        for (usize i = 0; i < 32; ++i) otk[i] = (u8)(0xC0u + i);

        u8 msg[257]{};
        for (usize i = 0; i < 257; ++i) msg[i] = (u8)(i * 17u + 3u);

        u8 t1[16]{};
        {
            cl::poly1305 p;
            p.init(otk);
            p.update(msg);
            p.final(t1);
        }

        u8 t2[16]{};
        {
            cl::poly1305 p;
            p.init(otk);

            // split in weird chunks to exercise partial buffering
            p.update(io::byte_view_n<1> { msg + 0 });
            p.update(io::byte_view_n<15>{ msg + 1  });
            p.update(io::byte_view_n<16>{ msg + 16 });
            p.update(io::byte_view_n<7> { msg + 32 });
            p.update(io::byte_view_n<64>{ msg + 39 });
            p.update(io::byte_view_n<154>{ msg + 103 });

            p.final(t2);
        }

        require_eq_bytes(t1, t2);
    }

    TEST_CASE("Poly1305: ct_equal works and is size-safe", "[poly1305][ct_equal]") {
        u8 a[16]{};
        u8 b[16]{};

        for (usize i = 0; i < 16; ++i) { a[i] = (u8)i; b[i] = (u8)i; }
        REQUIRE(cl::poly1305::ct_equal(io::as_view(a), io::as_view(b)));

        b[7] ^= 0x01;
        REQUIRE_FALSE(cl::poly1305::ct_equal(io::as_view(a), io::as_view(b)));
    }



} // namespace test_poly1305