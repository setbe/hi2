#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/crypto/chacha20.hpp"

// Tests use public API:
// - chacha20::init(byte_view_n<32> key, byte_view_n<12> nonce, u32 counter)
// - chacha20::keystream(byte_view_mut out)
// - chacha20::encrypt(byte_view in, byte_view_mut out)
// - chacha20::encrypt_inplace(byte_view_mut buf)

namespace test_chacha20 {

    using u8 = io::u8;

    static inline io::u8 hex_nibble(char c) noexcept {
        if (c >= '0' && c <= '9') return (io::u8)(c - '0');
        if (c >= 'a' && c <= 'f') return (io::u8)(10 + (c - 'a'));
        if (c >= 'A' && c <= 'F') return (io::u8)(10 + (c - 'A'));
        return 0xFF;
    }

    template<io::usize N>
    static inline void hex_to_bytes(const char(&hex)[N], io::u8(&out)[(N-1) / 2]) noexcept {
        static_assert(N >= 1, "hex literal required");
        static_assert(((N-1) % 2) == 0, "hex string must have even length");

        constexpr io::usize OUT_N = (N - 1) / 2;

        for (io::usize i = 0; i < OUT_N; ++i) {
            io::u8 hi = hex_nibble(hex[i*2 + 0]);
            io::u8 lo = hex_nibble(hex[i*2 + 1]);
            out[i] = (io::u8)((hi << 4) | lo);
        }
    }

    template<io::usize N>
    static inline void require_eq_bytes(const u8(&got)[N], const u8(&exp)[N]) {
        for (io::usize i=0; i < N; ++i) REQUIRE(got[i] == exp[i]);
    }

    static inline void require_eq_views(io::byte_view got, io::byte_view exp) {
        REQUIRE(got.size() == exp.size());
        for (io::usize i=0; i < got.size(); ++i) REQUIRE(got[i] == exp[i]);
    }

    // -------------------------------------------------------------------------
    // RFC 8439 vectors (ChaCha20)
    // - 2.3.2: ChaCha20 block function output (64-byte keystream block)
    // - 2.4.2: ChaCha20 encryption example
    // -------------------------------------------------------------------------

    TEST_CASE("ChaCha20: block function (RFC 8439)", "[chacha20][rfc8439][block]") {
        u8 key[32]{};
        hex_to_bytes(
            "000102030405060708090a0b0c0d0e0f"
            "101112131415161718191a1b1c1d1e1f",
            key
        );


        u8 nonce[12]{};
        hex_to_bytes("000000090000004a00000000", nonce);

        cl::chacha20 c;
        c.init(key, nonce, 1);

        u8 ks[64]{};
        c.keystream(io::as_view_mut(ks)); // 64 bytes

        u8 exp[64]{};
        hex_to_bytes(
            "10f1e7e4d13b5915500fdd1fa32071c4"
            "c7d1f4c733c068030422aa9ac3d46c4e"
            "d2826446079faa0914c2d705d98b02a2"
            "b5129cd1de164eb9cbd083e8a2503c4e",
            exp
        );

        require_eq_bytes(ks, exp);
    }

    TEST_CASE("ChaCha20: encrypt (RFC 8439)", "[chacha20][rfc8439][encrypt]") {
        u8 key[32]{};
        hex_to_bytes(
            "000102030405060708090a0b0c0d0e0f"
            "101112131415161718191a1b1c1d1e1f",
            key
        );

        u8 nonce[12]{};
        hex_to_bytes("000000000000004a00000000", nonce);

        cl::chacha20 c;
        c.init(key, nonce, 1);

        const char* msg =
            "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
        constexpr io::usize MSG_N = 114;

        u8 pt[MSG_N]{};
        for (io::usize i = 0; i < MSG_N; ++i) pt[i] = (u8)msg[i];

        u8 ct[MSG_N]{};
        c.encrypt(pt, ct);

        u8 exp[MSG_N]{};
        hex_to_bytes(
            "6e2e359a2568f98041ba0728dd0d6981"
            "e97e7aec1d4360c20a27afccfd9fae0b"
            "f91b65c5524733ab8f593dabcd62b357"
            "1639d624e65152ab8f530c359f0861d8"
            "07ca0dbf500d6a6156a38e088a22b65e"
            "52bc514d16ccf806818ce91ab7793736"
            "5af90bbf74a35be6b40b8eedf2785e42"
            "874d",
            exp
        );

        require_eq_views(ct, exp);
    }

    TEST_CASE("ChaCha20: encrypt_inplace equals encrypt and decrypts back", "[chacha20][inplace]") {
        u8 key[32]{};
        u8 nonce[12]{};
        for (io::usize i = 0; i < 32; ++i) key[i] = (u8)i;
        for (io::usize i = 0; i < 12; ++i) nonce[i] = (u8)(0xA0u + i);

        u8 msg[257]{};
        for (io::usize i = 0; i < 257; ++i) msg[i] = (u8)(i * 13u + 7u);

        u8 a[257]{};
        u8 b[257]{};
        for (io::usize i = 0; i < 257; ++i) { a[i] = msg[i]; b[i] = msg[i]; }

        cl::chacha20 c1;
        c1.init(key, nonce, 7);
        c1.encrypt(io::as_view(a), io::as_view_mut(a));

        cl::chacha20 c2;
        c2.init(key, nonce, 7);
        c2.encrypt_inplace(b);

        require_eq_views(a, b);

        // decrypt back (stream cipher == XOR twice)
        cl::chacha20 c3;
        c3.init(key, nonce, 7);
        c3.encrypt_inplace(b);

        require_eq_views(b, msg);
    }

} // namespace test_chacha20
