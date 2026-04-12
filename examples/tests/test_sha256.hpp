#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../../hi/crypto/sha256.hpp"

// Tests use public API:
// - sha256::update(const u8(&)[N])
// - sha256::final(u8(&)[N])
// - sha256::hmac(byte_view_n, byte_view_n, byte_view_mut_n)
// and io::view/as_view from `io.hpp`.

namespace test_sha256 {

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

        for (io::usize i=0; i < OUT_N; ++i) {
            io::u8 hi = hex_nibble(hex[i*2 + 0]);
            io::u8 lo = hex_nibble(hex[i*2 + 1]);
            out[i] = (io::u8)((hi << 4) | lo);
        }
    }

    template<io::usize N>
    static inline void hex_to_bytes(io::char_view view, io::u8(&out)[N]) noexcept {
        char buf[N*2 + (N?1:0)]{0};
        for (io::usize i=0; i < view.size(); ++i)
            buf[i] = view[i];
        hex_to_bytes(buf, out);
    }


    template<io::usize N>
    static inline void require_eq_bytes(const u8(&got)[N], const u8(&exp)[N]) {
        for (io::usize i = 0; i < N; ++i) REQUIRE(got[i] == exp[i]);
    }


    // -----------------------------------------------------------------------------
// SHA-256 vectors (RFC 6234 / FIPS 180-4)
// -----------------------------------------------------------------------------

    TEST_CASE("SHA-256: empty string (RFC 6234)", "[sha256][rfc6234][fips180-4]") {
        cl::sha256 h;

        u8 out[cl::sha256::DIGEST_BYTES]{};
        h.final(out);

        u8 exp[cl::sha256::DIGEST_BYTES]{};
        hex_to_bytes("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", exp);

        require_eq_bytes(out, exp);
    }

    TEST_CASE("SHA-256: 'abc' (RFC 6234)", "[sha256][rfc6234][fips180-4]") {
        cl::sha256 h;

        const u8 msg[] = { 'a','b','c' };
        h.update(msg);

        u8 out[cl::sha256::DIGEST_BYTES]{};
        h.final(out);

        u8 exp[cl::sha256::DIGEST_BYTES]{};
        hex_to_bytes("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", exp);

        require_eq_bytes(out, exp);
    }

    TEST_CASE("SHA-256: long vector (RFC 6234)", "[sha256][rfc6234][fips180-4]") {
        cl::sha256 h;

        // "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
        // Feed in chunks as C-arrays to match your overload set.
        const u8 a[] = { 'a','b','c','d','b','c','d','e','c','d','e','f','d','e','f','g' };
        const u8 b[] = { 'e','f','g','h','f','g','h','i','g','h','i','j','h','i','j','k' };
        const u8 c[] = { 'i','j','k','l','j','k','l','m','k','l','m','n','l','m','n','o' };
        const u8 d[] = { 'm','n','o','p','n','o','p','q' };
        h.update(a);
        h.update(b);
        h.update(c);
        h.update(d);

        u8 out[cl::sha256::DIGEST_BYTES]{};
        h.final(out);

        u8 exp[cl::sha256::DIGEST_BYTES]{};
        hex_to_bytes("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", exp);

        require_eq_bytes(out, exp);
    }

    // -----------------------------------------------------------------------------
    // HMAC-SHA-256 vectors (RFC 2104 mechanics; published test vectors in RFC 4868)
    // -----------------------------------------------------------------------------

    TEST_CASE("HMAC-SHA-256: test case 1 (RFC 4868)", "[hmac][sha256][rfc4868][rfc2104]") {
        u8 key[20]; for (auto& b : key) b = 0x0b;
        const u8 data[] = { 'H','i',' ','T','h','e','r','e' };
        u8 mac[cl::sha256::DIGEST_BYTES]{};

        cl::sha256::hmac(io::as_view(key), io::as_view(data), io::as_view_mut(mac));

        u8 exp[cl::sha256::DIGEST_BYTES]{};

        hex_to_bytes("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7", exp);
        require_eq_bytes(mac, exp);
    }

    TEST_CASE("HMAC-SHA-256: test case 2 (RFC 4868)", "[hmac][sha256][rfc4868][rfc2104]") {
        const u8 key[] = { 'J','e','f','e' };
        const u8 data[] = {
            'w','h','a','t',' ','d','o',' ','y','a',' ','w','a','n','t',' ',
            'f','o','r',' ','n','o','t','h','i','n','g','?'
        };

        u8 mac[cl::sha256::DIGEST_BYTES]{};
        cl::sha256::hmac(io::as_view(key), io::as_view(data), io::as_view_mut(mac));

        u8 exp[cl::sha256::DIGEST_BYTES]{};
        hex_to_bytes("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843", exp);

        require_eq_bytes(mac, exp);
    }

    TEST_CASE("HMAC-SHA-256: test case 3 (RFC 4868)", "[hmac][sha256][rfc4868][rfc2104]") {
        u8 key[20];  for (auto& b : key)  b = 0xaa;
        u8 data[50]; for (auto& b : data) b = 0xdd;

        u8 mac[cl::sha256::DIGEST_BYTES]{};
        cl::sha256::hmac(io::as_view(key), io::as_view(data), io::as_view_mut(mac));

        u8 exp[cl::sha256::DIGEST_BYTES]{};
        hex_to_bytes("773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe", exp);

        require_eq_bytes(mac, exp);
    }

    TEST_CASE("HMAC-SHA-256: key > block size (RFC 4868)", "[hmac][sha256][rfc4868][rfc2104]") {
        u8 key[131]; for (auto& b : key) b = 0xaa;

        const u8 data[] = {
            'T','e','s','t',' ','U','s','i','n','g',' ','L','a','r','g','e','r',' ',
            'T','h','a','n',' ','B','l','o','c','k','-','S','i','z','e',' ','K','e','y',' ',
            '-',' ','H','a','s','h',' ','K','e','y',' ','F','i','r','s','t'
        };

        u8 mac[cl::sha256::DIGEST_BYTES]{};
        cl::sha256::hmac(io::as_view(key), io::as_view(data), io::as_view_mut(mac));

        u8 exp[cl::sha256::DIGEST_BYTES]{};
        hex_to_bytes("60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54", exp);

        require_eq_bytes(mac, exp);
    }


    // -----------------------------------------------------------------------------
    // More SHA-256 vectors (FIPS 180-4 / RFC 6234) in one table-driven test
    // Input uses io::char_view (length auto-trimmed) -> io::byte_view
    // -----------------------------------------------------------------------------

    namespace {

        static inline io::byte_view as_bytes(io::char_view s) noexcept {
            return io::byte_view{ (const u8*)s.data(), s.size() };
        }

    } // anon

    TEST_CASE("SHA-256: many known vectors (RFC 6234 / FIPS 180-4)", "[sha256][vectors][rfc6234][fips180-4]") {
        struct vec {
            io::char_view msg;
            io::char_view hex;
        };

        // NOTE: io::char_view ctor trims '\0', so size is correct automatically.
        static const vec cases[] = {
            { "",  "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" },
            { "abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" },

            // Common public vectors (widely used in test suites)
            { "The quick brown fox jumps over the lazy dog",
              "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592" },

            { "The quick brown fox jumps over the lazy dog.",
              "ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c" },

              // RFC 6234 / FIPS 180-4 long message
              { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1" },
        };

        for (auto tc : cases) {
            cl::sha256 h;

            h.update(as_bytes(tc.msg));

            u8 out[cl::sha256::DIGEST_BYTES]{};
            h.final(out);

            u8 exp[cl::sha256::DIGEST_BYTES]{};
            hex_to_bytes(tc.hex, exp);
            require_eq_bytes(out, exp);
        }

        // FIPS 180-4: 1,000,000 times 'a'
        {
            cl::sha256 h;

            u8 block[64];
            for (auto& b : block) b = (u8)'a';

            // 64 * 15625 = 1,000,000
            for (int i = 0; i < 15625; ++i)
                h.update(block);

            u8 out[cl::sha256::DIGEST_BYTES]{};
            h.final(out);

            u8 exp[cl::sha256::DIGEST_BYTES]{};
            hex_to_bytes(
                "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0",
                exp
            );
            require_eq_bytes(out, exp);
        }
    }
} // namespace test_sha256