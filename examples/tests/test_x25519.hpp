#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/crypto/x25519.hpp"

// Tests use public API:
// - cl::x25519::generate_public(priv32, pub32)
// - cl::x25519::shared_secret(priv32, peer_pub32, out32)

namespace test_x25519 {
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
    // RFC 7748 test vectors (X25519)
    // -------------------------------------------------------------------------

    TEST_CASE("X25519: scalar mult test vector #1 (RFC 7748)", "[x25519][rfc7748][vec1]") {
        u8 scalar[32]{};
        u8 u[32]{};
        u8 exp[32]{};

        hex_to_bytes("a546e36bf0527c9d3b16154b82465edd62144c0ac1fc5a18506a2244ba449ac4", scalar);
        hex_to_bytes("e6db6867583030db3594c1a424b15f7c726624ec26b3353b10a903a6d0ab1c4c", u);
        hex_to_bytes("c3da55379de9c6908e94ea4df28d084f32eccf03491c71f754b4075577a28552", exp);

        u8 out[32]{};
        const bool ok = cl::x25519::shared_secret(io::as_view(scalar), io::as_view(u), io::as_view_mut(out));
        REQUIRE(ok);
        require_eq_bytes(out, exp);
    }

    TEST_CASE("X25519: scalar mult test vector #2 (RFC 7748)", "[x25519][rfc7748][vec2]") {
        u8 scalar[32]{};
        u8 u[32]{};
        u8 exp[32]{};

        hex_to_bytes("4b66e9d4d1b4673c5ad22691957d6af5c11b6421e0ea01d42ca4169e7918ba0d", scalar);
        hex_to_bytes("e5210f12786811d3f4b7959d0538ae2c31dbe7106fc03c3efc4cd549c715a493", u);
        hex_to_bytes("95cbde9476e8907d7aade45cb4b873f88b595a68799fa152e6f8f7647aac7957", exp);

        u8 out[32]{};
        const bool ok = cl::x25519::shared_secret(io::as_view(scalar), io::as_view(u), io::as_view_mut(out));
        REQUIRE(ok);
        require_eq_bytes(out, exp);
    }

    TEST_CASE("X25519: Alice/Bob key exchange (RFC 7748)", "[x25519][rfc7748][kex]") {
        u8 alice_priv[32]{};
        u8 alice_pub_exp[32]{};
        u8 bob_priv[32]{};
        u8 bob_pub_exp[32]{};
        u8 shared_exp[32]{};

        hex_to_bytes("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a", alice_priv);
        hex_to_bytes("8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a", alice_pub_exp);

        hex_to_bytes("5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb", bob_priv);
        hex_to_bytes("de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f", bob_pub_exp);

        hex_to_bytes("4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742", shared_exp);

        // generate_public()
        u8 alice_pub[32]{};
        cl::x25519::generate_public(io::as_view(alice_priv), io::as_view_mut(alice_pub));
        require_eq_bytes(alice_pub, alice_pub_exp);

        u8 bob_pub[32]{};
        cl::x25519::generate_public(io::as_view(bob_priv), io::as_view_mut(bob_pub));
        require_eq_bytes(bob_pub, bob_pub_exp);

        // shared_secret() both ways
        u8 s1[32]{};
        u8 s2[32]{};

        REQUIRE(cl::x25519::shared_secret(io::as_view(alice_priv), io::as_view(bob_pub), io::as_view_mut(s1)));
        REQUIRE(cl::x25519::shared_secret(io::as_view(bob_priv), io::as_view(alice_pub), io::as_view_mut(s2)));

        require_eq_bytes(s1, shared_exp);
        require_eq_bytes(s2, shared_exp);
    }

    

} // namespace test_x25519