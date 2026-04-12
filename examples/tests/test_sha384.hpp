#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/crypto/sha384.hpp"

// Tests use public API:
// - sha384::update(const u8(&)[N])
// - sha384::final(u8(&)[N])
// - sha384::hmac(byte_view_n, byte_view_n, byte_view_mut_n)

namespace test_sha384 {

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

        constexpr io::usize OUT_N = (N-1) / 2;

        for (io::usize i = 0; i < OUT_N; ++i) {
            io::u8 hi = hex_nibble(hex[i*2 + 0]);
            io::u8 lo = hex_nibble(hex[i*2 + 1]);
            out[i] = (io::u8)((hi << 4) | lo);
        }
    }

    template<io::usize N>
    static inline void hex_to_bytes(io::char_view view, io::u8(&out)[N]) noexcept {
        char buf[N * 2 + (N ? 1 : 0)]{ 0 };
        for (io::usize i=0; i < view.size(); ++i) buf[i] = view[i];
        hex_to_bytes(buf, out);
    }

    template<io::usize N>
    static inline void require_eq_bytes(const u8(&got)[N], const u8(&exp)[N]) {
        for (io::usize i=0; i < N; ++i) REQUIRE(got[i] == exp[i]);
    }

    static inline io::byte_view as_bytes(io::char_view s) noexcept {
        return io::byte_view{ (const u8*)s.data(), s.size() };
    }

    // -----------------------------------------------------------------------------
    // SHA-384 vectors (RFC 6234 / FIPS 180-4)
    // -----------------------------------------------------------------------------

    TEST_CASE("SHA-384: empty string (RFC 6234)", "[sha384][rfc6234][fips180-4]") {
        cl::sha384 h;

        u8 out[cl::sha384::DIGEST_BYTES]{};
        h.final(out);

        u8 exp[cl::sha384::DIGEST_BYTES]{};
        hex_to_bytes(
            "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da"
            "274edebfe76f65fbd51ad2f14898b95b",
            exp
        );

        require_eq_bytes(out, exp);
    }

    TEST_CASE("SHA-384: 'abc' (RFC 6234)", "[sha384][rfc6234][fips180-4]") {
        cl::sha384 h;

        const u8 msg[] = { 'a','b','c' };
        h.update(msg);

        u8 out[cl::sha384::DIGEST_BYTES]{};
        h.final(out);

        u8 exp[cl::sha384::DIGEST_BYTES]{};
        hex_to_bytes(
            "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded163"
            "1a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7",
            exp
        );

        require_eq_bytes(out, exp);
    }

    TEST_CASE("SHA-384: long vector (RFC 6234)", "[sha384][rfc6234][fips180-4]") {
        cl::sha384 h;

        // "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
        // "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
        const u8 a[] = { 'a','b','c','d','e','f','g','h','b','c','d','e','f','g','h','i' };
        const u8 b[] = { 'c','d','e','f','g','h','i','j','d','e','f','g','h','i','j','k' };
        const u8 c[] = { 'e','f','g','h','i','j','k','l','f','g','h','i','j','k','l','m' };
        const u8 d[] = { 'g','h','i','j','k','l','m','n','h','i','j','k','l','m','n','o' };
        const u8 e[] = { 'i','j','k','l','m','n','o','p','j','k','l','m','n','o','p','q' };
        const u8 f[] = { 'k','l','m','n','o','p','q','r','l','m','n','o','p','q','r','s' };
        const u8 g[] = { 'm','n','o','p','q','r','s','t','n','o','p','q','r','s','t','u' };

        h.update(a); h.update(b); h.update(c); h.update(d);
        h.update(e); h.update(f); h.update(g);

        u8 out[cl::sha384::DIGEST_BYTES]{};
        h.final(out);

        u8 exp[cl::sha384::DIGEST_BYTES]{};
        hex_to_bytes(
            "09330c33f71147e83d192fc782cd1b4753111b173b3b05d2"
            "2fa08086e3b0f712fcc7c71a557e2db966c3e9fa91746039",
            exp
        );

        require_eq_bytes(out, exp);
    }

    // -----------------------------------------------------------------------------
    // HMAC-SHA-384 vectors (RFC 2104 mechanics; published test vectors in RFC 4231)
    // -----------------------------------------------------------------------------

    TEST_CASE("HMAC-SHA-384: test case 1 (RFC 4231)", "[hmac][sha384][rfc4231][rfc2104]") {
        u8 key[20]; for (auto& b : key) b = 0x0b;
        const u8 data[] = { 'H','i',' ','T','h','e','r','e' };

        u8 mac[cl::sha384::DIGEST_BYTES]{};
        cl::sha384::hmac(io::as_view(key), io::as_view(data), io::as_view_mut(mac));

        u8 exp[cl::sha384::DIGEST_BYTES]{};
        hex_to_bytes(
            "afd03944d84895626b0825f4ab46907f15f9dadbe4101ec6"
            "82aa034c7cebc59cfaea9ea9076ede7f4af152e8b2fa9cb6",
            exp
        );

        require_eq_bytes(mac, exp);
    }

    TEST_CASE("HMAC-SHA-384: test case 2 (RFC 4231)", "[hmac][sha384][rfc4231][rfc2104]") {
        const u8 key[] = { 'J','e','f','e' };
        const u8 data[] = {
            'w','h','a','t',' ','d','o',' ','y','a',' ','w','a','n','t',' ',
            'f','o','r',' ','n','o','t','h','i','n','g','?'
        };

        u8 mac[cl::sha384::DIGEST_BYTES]{};
        cl::sha384::hmac(io::as_view(key), io::as_view(data), io::as_view_mut(mac));

        u8 exp[cl::sha384::DIGEST_BYTES]{};
        hex_to_bytes(
            "af45d2e376484031617f78d2b58a6b1b9c7ef464f5a01b47"
            "e42ec3736322445e8e2240ca5e69e2c78b3239ecfab21649",
            exp
        );

        require_eq_bytes(mac, exp);
    }

    TEST_CASE("HMAC-SHA-384: test case 3 (RFC 4231)", "[hmac][sha384][rfc4231][rfc2104]") {
        u8 key[20];  for (auto& b : key)  b = 0xaa;
        u8 data[50]; for (auto& b : data) b = 0xdd;

        u8 mac[cl::sha384::DIGEST_BYTES]{};
        cl::sha384::hmac(io::as_view(key), io::as_view(data), io::as_view_mut(mac));

        u8 exp[cl::sha384::DIGEST_BYTES]{};
        hex_to_bytes(
            "88062608d3e6ad8a0aa2ace014c8a86f0aa635d947ac9febe83ef4e55966144b"
            "2a5ab39dc13814b94e3ab6e101a34f27",
            exp
        );

        require_eq_bytes(mac, exp);
    }

    TEST_CASE("HMAC-SHA-384: key > block size (RFC 4231)", "[hmac][sha384][rfc4231][rfc2104]") {
        u8 key[131]; for (auto& b : key) b = 0xaa;

        const u8 data[] = {
            'T','e','s','t',' ','U','s','i','n','g',' ','L','a','r','g','e','r',' ',
            'T','h','a','n',' ','B','l','o','c','k','-','S','i','z','e',' ','K','e','y',' ',
            '-',' ','H','a','s','h',' ','K','e','y',' ','F','i','r','s','t'
        };

        u8 mac[cl::sha384::DIGEST_BYTES]{};
        cl::sha384::hmac(io::as_view(key), io::as_view(data), io::as_view_mut(mac));

        // RFC 4231 test case 6 (SHA-384)
        u8 exp[cl::sha384::DIGEST_BYTES]{};
        hex_to_bytes(
            "4ece084485813e9088d2c63a041bc5b4"
            "4f9ef1012a2b588f3cd11f05033ac4c6"
            "0c2ef6ab4030fe8296248df163f44952",
            exp
        );

        require_eq_bytes(mac, exp);
    }

    // -----------------------------------------------------------------------------
    // More SHA-384 vectors in one table-driven test
    // -----------------------------------------------------------------------------

    TEST_CASE("SHA-384: many known vectors (RFC 6234 / FIPS 180-4)", "[sha384][vectors][rfc6234][fips180-4]") {
        struct vec { io::char_view msg; io::char_view hex; };

        static const vec cases[] = {
            { "",
              "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da"
              "274edebfe76f65fbd51ad2f14898b95b" },

            { "abc",
              "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded163"
              "1a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7" },

            { "The quick brown fox jumps over the lazy dog",
              "ca737f1014a48f4c0b6dd43cb177b0afd9e5169367544c494011e3317dbf9a509cb1e5dc1e85a941bbee3d7f2afbc9b1" },

            { "The quick brown fox jumps over the lazy dog.",
              "ed892481d8272ca6df370bf706e4d7bc1b5739fa2177aae6c50e946678718fc67a7af2819a021c2fc34e91bdb63409d7" },

            { "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
              "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
              "09330c33f71147e83d192fc782cd1b4753111b173b3b05d2"
              "2fa08086e3b0f712fcc7c71a557e2db966c3e9fa91746039" },
        };

        for (auto tc : cases) {
            cl::sha384 h;
            h.update(as_bytes(tc.msg));

            u8 out[cl::sha384::DIGEST_BYTES]{};
            h.final(out);

            u8 exp[cl::sha384::DIGEST_BYTES]{};
            hex_to_bytes(tc.hex, exp);

            require_eq_bytes(out, exp);
        }

        // FIPS 180-4: 1,000,000 times 'a' (SHA-384)
        {
            cl::sha384 h;

            u8 block[128];
            for (auto& b : block) b = (u8)'a';

            // 128 * 7812 = 999,936; remainder 64 -> total 1,000,000
            for (int i = 0; i < 7812; ++i) h.update(block);
            u8 rem[64]; for (auto& b : rem) b = (u8)'a';
            h.update(rem);

            u8 out[cl::sha384::DIGEST_BYTES]{};
            h.final(out);

            u8 exp[cl::sha384::DIGEST_BYTES]{};
            hex_to_bytes(
                "9d0e1809716474cb086e834e310a4a1ced149e9c00f248527972cec5704c2a5b"
                "07b8b3dc38ecc4ebae97ddd87f3d8985",
                exp
            );

            require_eq_bytes(out, exp);
        }
    }

} // namespace test_sha384