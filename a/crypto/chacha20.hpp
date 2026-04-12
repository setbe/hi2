// RFC 8439: ChaCha20 and Poly1305 for IETF Protocols
// Notes:
// - This is the IETF ChaCha20 (32-byte key, 12-byte nonce, 32-bit counter).
// - Encrypt/decrypt are identical (XOR with keystream).
#pragma once

#include "../a.hpp"
#include "detail/load_store.hpp"
#include "detail/wipe.hpp"

namespace a {
namespace crypto {
    struct chacha20 {
        using u8 = a::u8;
        using u32 = a::u32;
        using usize = a::usize;

        using byte_view = a::byte_view;
        using byte_view_mut = a::byte_view_mut;
        template<usize N> using byte_view_n = a::byte_view_n<N>;
        template<usize N> using byte_view_mut_n = a::byte_view_mut_n<N>;

        // Constants
        static A_CONSTEXPR_VAR usize BLOCK_BYTES = 64;   // 1 block = 64 bytes
        static A_CONSTEXPR_VAR usize STATE_WORDS = 16;   // 16 x u32
        static A_CONSTEXPR_VAR usize KEY_BYTES = 32;   // 256-bit key
        static A_CONSTEXPR_VAR usize NONCE_BYTES = 12;   // 96-bit IETF nonce

        static_assert(BLOCK_BYTES == 64, "chacha20: block is 64 bytes");
        static_assert(KEY_BYTES == 32, "chacha20: key is 32 bytes");
        static_assert(NONCE_BYTES == 12, "chacha20: nonce is 12 bytes");

        // RAII
        explicit A_CONSTEXPR chacha20() noexcept
            : _state{ 0 }, _keystream{ 0 }, _ks_off{ 0 } {}
        ~chacha20() noexcept { wipe(); }

    private:
        A_NODISCARD A_CONSTEXPR static u32 rotl(u32 v, u32 n) noexcept {
            return (v << n) | (v >> (32u - n));
        }

        A_CONSTEXPR static void quarter_round(u32& a, u32& b, u32& c, u32& d) noexcept {
            a += b; d ^= a; d = rotl(d, 16);
            c += d; b ^= c; b = rotl(b, 12);
            a += b; d ^= a; d = rotl(d, 8);
            c += d; b ^= c; b = rotl(b, 7);
        }

        void keystream_block() noexcept {
            u32 x[STATE_WORDS];
            for (usize i = 0; i < STATE_WORDS; ++i) x[i] = _state[i];

            // 20 rounds = 10 double-rounds
            for (int r = 0; r < 10; ++r) {
                // columns
                quarter_round(x[0], x[4], x[8], x[12]);
                quarter_round(x[1], x[5], x[9], x[13]);
                quarter_round(x[2], x[6], x[10], x[14]);
                quarter_round(x[3], x[7], x[11], x[15]);
                // diagonals
                quarter_round(x[0], x[5], x[10], x[15]);
                quarter_round(x[1], x[6], x[11], x[12]);
                quarter_round(x[2], x[7], x[8], x[13]);
                quarter_round(x[3], x[4], x[9], x[14]);
            }

            // add original state and serialize
            for (usize i = 0; i < STATE_WORDS; ++i) {
                u32 res = x[i] + _state[i];
                detail::store32_le(_keystream + (i * 4u), res);
            }

            // increment counter (word 12), wrap modulo 2^32
            _state[12] += 1u;
            _ks_off = 0;
        }

        void xor_stream(byte_view in, byte_view_mut out) noexcept {
            if (!in || !out) return;
            if (out.size() < in.size()) return; // debug build could assert; keep it no-op in release

            usize pos = 0;
            const usize n = in.size();

            while (pos < n) {
                if (_ks_off == BLOCK_BYTES) {
                    // should not happen (we reset to 0 after block), but keep invariant
                    _ks_off = 0;
                }
                if (_ks_off == 0) keystream_block();

                usize avail = BLOCK_BYTES - _ks_off;
                usize take = (n - pos < avail) ? (n - pos) : avail;

                for (usize i = 0; i < take; ++i)
                    out[pos + i] = (u8)(in[pos + i] ^ _keystream[_ks_off + i]);

                pos += take;
                _ks_off += take;
                if (_ks_off == BLOCK_BYTES) _ks_off = 0;
            }
        }
        
        void init(byte_view key32, byte_view nonce12, u32 counter = 1u) noexcept {
            if (!key32 || key32.size() != KEY_BYTES) return;
            if (!nonce12 || nonce12.size() != NONCE_BYTES) return;

            // "expand 32-byte k"
            _state[0] = 0x61707865u; // "expa"
            _state[1] = 0x3320646eu; // "nd 3"
            _state[2] = 0x79622d32u; // "2-by"
            _state[3] = 0x6b206574u; // "te k"

            // key words 4..11: load as little-endian 32-bit words
            for (usize i = 0; i < 8; ++i)
                _state[4+i] = detail::load32_le(key32.data() + 4*i);

            // counter word 12
            _state[12] = counter;

            // nonce words 13..15
            _state[13] = detail::load32_le(nonce12.data() + 0);
            _state[14] = detail::load32_le(nonce12.data() + 4);
            _state[15] = detail::load32_le(nonce12.data() + 8);

            _ks_off = 0;
        }
        void keystream(byte_view_mut out) noexcept {
            if (!out) return;
            usize pos = 0;
            while (pos < out.size()) {
                if (_ks_off == 0) keystream_block();
                usize avail = BLOCK_BYTES - _ks_off;
                usize take = (out.size() - pos < avail) ? (out.size() - pos) : avail;
                for (usize i = 0; i < take; ++i)
                    out[pos + i] = _keystream[_ks_off + i];
                pos += take;
                _ks_off += take;
                if (_ks_off == BLOCK_BYTES) _ks_off = 0;
            }
        }
        void encrypt(byte_view in, byte_view_mut out) noexcept { xor_stream(in, out); }
        void encrypt_inplace(byte_view_mut buf) noexcept {
            if (!buf) return;
            xor_stream(byte_view{ buf.data(), buf.size() }, buf);
        }

    public:
        // --------------------------------------------------------------------
        //                              Public API
        // --------------------------------------------------------------------

        // Reset logical state for next reuse
        void reset() noexcept {
            for (usize i = 0; i < STATE_WORDS; ++i) _state[i] = 0;
            for (usize i = 0; i < BLOCK_BYTES; ++i) _keystream[i] = 0;
            _ks_off = 0;
        }

        // Wipe cryptography secrets
        void wipe() noexcept {
            detail::secure_zero(_state, sizeof(_state));
            detail::secure_zero(_keystream, sizeof(_keystream));
            detail::secure_zero(&_ks_off, sizeof(_ks_off));
            _ks_off = 0;
        }

        // --- size-safe init ---

        // Initialize state = constants | key | counter | nonce
        // Counter default in many protocols is 1 (per RFC 8439 AEAD).
        template<usize NK, usize NN>
        void init(byte_view_n<NK> key, byte_view_n<NN> nonce, u32 counter = 1u) noexcept {
            static_assert(NK == KEY_BYTES, "chacha20::init: key must be 32 bytes");
            static_assert(NN == NONCE_BYTES, "chacha20::init: nonce must be 12 bytes");
            init((byte_view)key.v, (byte_view)nonce.v, counter);
        }
        void init(const u8(&key)[KEY_BYTES], const u8(&nonce)[NONCE_BYTES], u32 counter = 1u) noexcept {
            init(a::as_view(key), a::as_view(nonce), counter);
        }

        // --- size-safe keystream ---

        // Generate keystream bytes into `out` (no XOR).
        template<usize N> void keystream(byte_view_mut_n<N> out) noexcept { keystream((byte_view_mut)out.v); }
        template<usize N> void keystream(u8(&out)[N]) noexcept { keystream(byte_view_mut{ out, N }); }

        // --- size-safe encrypt/decrypt ---

        // Encrypt/decrypt: out = in XOR keystream
        template<usize N> void encrypt(byte_view_n<N> in, byte_view_mut_n<N> out) noexcept { encrypt((byte_view)in.v, (byte_view_mut)out.v); }
        template<usize N> void encrypt(const u8(&in)[N], u8(&out)[N]) noexcept { encrypt(a::as_view(in), a::as_view_mut(out)); }

        // --- size-safe in-place ---

        // In-place
        template<usize N> void encrypt_inplace(byte_view_mut_n<N> buf) noexcept { encrypt_inplace((byte_view_mut)buf.v); }
        template<usize N> void encrypt_inplace(u8(&buf)[N]) noexcept { encrypt_inplace(a::as_view_mut(buf)); }

    private:
        u32  _state[STATE_WORDS];      // constants|key|counter|nonce
        u8   _keystream[BLOCK_BYTES];  // last block
        usize _ks_off;                 // offset in _keystream
    }; // struct chacha20
} // namespace crypto
} // namespace a
