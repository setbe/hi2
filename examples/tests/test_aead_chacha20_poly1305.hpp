#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/crypto/aead_chacha20_poly1305.hpp"

// Tests cover:
// - RFC 8439 AEAD example (seal/open vectors)
// - tamper detection (tag, aad, ciphertext)
// - empty aad / empty plaintext edge cases
// - size-safe public API usage only

namespace test_aead_chacha20_poly1305 {
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

    template<usize N>
    static inline void require_ne_bytes(const u8(&a)[N], const u8(&b)[N]) {
        bool any = false;
        for (usize i = 0; i < N; ++i) any |= (a[i] != b[i]);
        REQUIRE(any);
    }

    // -------------------------------------------------------------------------
    // RFC 8439 vectors (AEAD ChaCha20-Poly1305)
    // Section 2.8.2 (example and test vector)
    // -------------------------------------------------------------------------

    TEST_CASE("AEAD: seal/open (RFC 8439 example)", "[aead][rfc8439][seal][open]") {
        // Key: 80818283... (32 bytes)
        u8 key[32]{};
        hex_to_bytes(
            "808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f",
            key
        );

        // Nonce: 070000004041424344454647 (12 bytes)
        u8 nonce[12]{};
        hex_to_bytes("070000004041424344454647", nonce);

        // AAD: 50515253c0c1c2c3c4c5c6c7 (12 bytes)
        u8 aad[12]{};
        hex_to_bytes("50515253c0c1c2c3c4c5c6c7", aad);

        // Plaintext (114 bytes): RFC text
        const char* msg =
            "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
        constexpr usize PT_N = 114;

        u8 pt[PT_N]{};
        for (usize i = 0; i < PT_N; ++i) pt[i] = (u8)msg[i];

        // Expected ciphertext (114 bytes)
        u8 exp_ct[PT_N]{};
        hex_to_bytes(
            "d31a8d34648e60db7b86afbc53ef7ec2a4aded51296e08fea9e2b5a736ee62d63dbea45e8ca9"
            "671282fafb69da92728b1a71de0a9e060b2905d6a5b67ecd3b3692ddbd7f2d778b8c9803aee32"
            "8091b58fab324e4fad675945585808b4831d7bc3ff4def08e4b7a9de576d26586cec64b6116",
            exp_ct
        );

        // Expected tag (16 bytes)
        u8 exp_tag[16]{};
        hex_to_bytes("1ae10b594f09e26a7e902ecbd0600691", exp_tag);

        // Seal
        u8 ct[PT_N]{};
        u8 tag[16]{};
        cl::aead_chacha20_poly1305::seal(key, nonce, aad, pt, ct, tag);

        require_eq_bytes(ct, exp_ct);
        require_eq_bytes(tag, exp_tag);

        // Open
        u8 dec[PT_N]{};
        const bool ok = cl::aead_chacha20_poly1305::open(key, nonce, aad, ct, tag, dec);
        REQUIRE(ok);
        require_eq_bytes(dec, pt);
    }

    TEST_CASE("AEAD: tamper detection (tag/aad/ciphertext)", "[aead][tamper]") {
        u8 key[32]{};
        u8 nonce[12]{};
        for (usize i = 0; i < 32; ++i) key[i] = (u8)(0x10u + i);
        for (usize i = 0; i < 12; ++i) nonce[i] = (u8)(0xA0u + i);

        u8 aad[13]{};
        for (usize i = 0; i < 13; ++i) aad[i] = (u8)(0x50u + i);

        u8 pt[257]{};
        for (usize i = 0; i < 257; ++i) pt[i] = (u8)(i * 7u + 3u);

        u8 ct[257]{};
        u8 tag[16]{};
        cl::aead_chacha20_poly1305::seal(key, nonce, aad, pt, ct, tag);

        // Wrong tag
        {
            u8 bad_tag[16]{};
            for (usize i = 0; i < 16; ++i) bad_tag[i] = tag[i];
            bad_tag[0] ^= 0x01;

            u8 out[257]{};
            REQUIRE_FALSE(cl::aead_chacha20_poly1305::open(key, nonce, aad, ct, bad_tag, out));
        }

        // Wrong AAD
        {
            u8 bad_aad[13]{};
            for (usize i = 0; i < 13; ++i) bad_aad[i] = aad[i];
            bad_aad[12] ^= 0x80;

            u8 out[257]{};
            REQUIRE_FALSE(cl::aead_chacha20_poly1305::open(key, nonce, bad_aad, ct, tag, out));
        }

        // Wrong ciphertext
        {
            u8 bad_ct[257]{};
            for (usize i = 0; i < 257; ++i) bad_ct[i] = ct[i];
            bad_ct[100] ^= 0x40;

            u8 out[257]{};
            REQUIRE_FALSE(cl::aead_chacha20_poly1305::open(key, nonce, aad, bad_ct, tag, out));
        }
    }

    static IO_CONSTEXPR io::byte_view_n<0> empty_bytes() noexcept { return { io::byte_view{ nullptr, 0 } }; }
    static IO_CONSTEXPR io::byte_view_mut_n<0> empty_bytes_mut() noexcept { return { io::byte_view_mut{ nullptr, 0 } }; }

    TEST_CASE("AEAD: empty aad / empty plaintext edge cases", "[aead][edge]") {
        u8 key[32]{};
        u8 nonce[12]{};
        for (usize i = 0; i < 32; ++i) key[i] = (u8)i;
        for (usize i = 0; i < 12; ++i) nonce[i] = (u8)(0xF0u + i);

        // empty AAD, non-empty PT
        {
            u8 pt[33]{};
            for (usize i = 0; i < 33; ++i) pt[i] = (u8)(0x33u + i);

            u8 ct[33]{};
            u8 tag[16]{};
            cl::aead_chacha20_poly1305::seal(io::as_view(key),
                                             io::as_view(nonce),
                                             /* aad */ empty_bytes(),
                                             io::as_view(pt),
                                             io::as_view_mut(ct),
                                             io::as_view_mut(tag));

            u8 dec[33]{};
            REQUIRE(cl::aead_chacha20_poly1305::open(io::as_view(key),
                                                     io::as_view(nonce),
                                                     /* aad */ empty_bytes(),
                                                     io::as_view(ct),
                                                     io::as_view(tag),
                                                     io::as_view_mut(dec)));
            require_eq_bytes(dec, pt);
        }

        // non-empty AAD, empty PT
        {
            u8 aad[7]{};
            for (usize i = 0; i < 7; ++i) aad[i] = (u8)(0x70u + i);

            u8 tag[16]{};
            cl::aead_chacha20_poly1305::seal(io::as_view(key),
                                             io::as_view(nonce),
                                             io::as_view(aad),
                                             /* pt */ empty_bytes(),
                                             /* ct */ empty_bytes_mut(),
                                             io::as_view_mut(tag));

            REQUIRE(cl::aead_chacha20_poly1305::open(io::as_view(key),
                                                     io::as_view(nonce),
                                                     io::as_view(aad),
                                                     /* ct */ empty_bytes(),
                                                     io::as_view(tag),
                                                     /* pt */ empty_bytes_mut()));
        }

        // empty AAD, empty PT
        {
            u8 tag[16]{};
            cl::aead_chacha20_poly1305::seal(io::as_view(key),
                                             io::as_view(nonce),
                                             /* aad */ empty_bytes(),
                                             /* pt */ empty_bytes(),
                                             /* ct */ empty_bytes_mut(),
                                             io::as_view_mut(tag));

            REQUIRE(cl::aead_chacha20_poly1305::open(io::as_view(key),
                                                     io::as_view(nonce),
                                                     /* aad */ empty_bytes(),
                                                     /* ct */ empty_bytes(),
                                                     io::as_view(tag),
                                                     /* pt */ empty_bytes_mut()));
        }
    }

    TEST_CASE("AEAD: zero-length with non-null pointers", "[aead][edge][nonnull0]") {
        u8 key[32]{}, nonce[12]{};
        for (usize i = 0; i < 32; ++i) key[i] = (u8)i;
        for (usize i = 0; i < 12; ++i) nonce[i] = (u8)(0xE0u + i);

        u8 dummy_aad[1]{ 0xAA };
        u8 dummy_pt[1]{ 0xBB };
        u8 dummy_ct[1]{ 0xCC };

        u8 tag1[16]{};
        cl::aead_chacha20_poly1305::seal(io::as_view(key), io::as_view(nonce),
            io::byte_view_n<0>{ dummy_aad },
            io::byte_view_n<0>{ dummy_pt },
            io::byte_view_mut_n<0>{ dummy_ct },
            io::as_view_mut(tag1));

        REQUIRE(cl::aead_chacha20_poly1305::open(io::as_view(key), io::as_view(nonce),
            io::byte_view_n<0>{ dummy_aad },
            io::byte_view_n<0>{ dummy_ct},
            io::as_view(tag1),
            io::byte_view_mut_n<0>{ dummy_pt }));
    }

    TEST_CASE("AEAD: in-place encrypt/decrypt", "[aead][edge][inplace]") {
        u8 key[32]{}, nonce[12]{}, aad[8]{};
        for (usize i = 0; i < 32; ++i) key[i] = (u8)(0x20u + i);
        for (usize i = 0; i < 12; ++i) nonce[i] = (u8)(0x90u + i);
        for (usize i = 0; i < 8; ++i) aad[i] = (u8)(0x40u + i);

        u8 buf[64]{};
        for (usize i = 0; i < 64; ++i) buf[i] = (u8)(i * 5u + 1u);

        u8 orig[64]{};
        for (usize i = 0; i < 64; ++i) orig[i] = buf[i];

        u8 tag[16]{};
        cl::aead_chacha20_poly1305::seal(key, nonce, aad, buf, buf, tag); // in-place

        REQUIRE(cl::aead_chacha20_poly1305::open(key, nonce, aad, buf, tag, buf)); // in-place
        require_eq_bytes(buf, orig);
    }


} // namespace test_aead_chacha20_poly1305
