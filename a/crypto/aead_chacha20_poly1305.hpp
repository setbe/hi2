// RFC 8439: ChaCha20-Poly1305 AEAD
#pragma once

#include "../a.hpp"
#include "detail/load_store.hpp"
#include "detail/wipe.hpp"
#include "chacha20.hpp"
#include "poly1305.hpp"

namespace a {
namespace crypto {
    struct aead_chacha20_poly1305 {
        using u8 = a::u8;
        using u32 = a::u32;
        using u64 = a::u64;
        using usize = a::usize;

        using byte_view = a::byte_view;
        using byte_view_mut = a::byte_view_mut;
        template<usize N> using byte_view_n = a::byte_view_n<N>;
        template<usize N> using byte_view_mut_n = a::byte_view_mut_n<N>;

        // Constants
        static A_CONSTEXPR_VAR usize KEY_BYTES = a::crypto::chacha20::KEY_BYTES;      // 32
        static A_CONSTEXPR_VAR usize NONCE_BYTES = a::crypto::chacha20::NONCE_BYTES;  // 12
        static A_CONSTEXPR_VAR usize TAG_BYTES = a::crypto::poly1305::TAG_BYTES;      // 16

        static_assert(KEY_BYTES   == 32, "aead: key is 32 bytes");
        static_assert(NONCE_BYTES == 12, "aead: nonce is 12 bytes");
        static_assert(TAG_BYTES   == 16, "aead: tag is 16 bytes");

        // RAII (no state; still keep for symmetry / future extension)
        explicit A_CONSTEXPR aead_chacha20_poly1305() noexcept = default;
        ~aead_chacha20_poly1305() noexcept = default;

    private:
        A_CONSTEXPR static usize pad16(usize n) noexcept {
            const usize r = (n & 15u);
            return (r == 0u) ? 0u : (16u - r);
        }

        
    public:
        // --------------------------------------------------------------------
        //                              Public API
        // --------------------------------------------------------------------

        // Build Poly1305 tag for AEAD (RFC 8439):
        // tag = Poly1305( otk, aad || pad16 || ciphertext || pad16 || le64(aad_len) || le64(ct_len)
        template<usize NAAD, usize NPT>
        static void mac_rfc8439(byte_view_n<KEY_BYTES>   key,
                                byte_view_n<NONCE_BYTES> nonce,
                                byte_view_n<NAAD>        aad,
                                byte_view_n<NPT>         ciphertext,
                                byte_view_mut_n<TAG_BYTES> tag_out) noexcept {
            u8 zeros16[16]{};
            a::crypto::poly1305 p;

            u8 otk[a::crypto::poly1305::OTKEY_BYTES]{};
            a::crypto::poly1305::derive_otk_from_chacha20(key, nonce, a::as_view_mut(otk));
            p.init(otk);

            if (aad.len()) p.update(aad);
            const usize pad_aad = pad16(aad.len());
            if (pad_aad) p.update(a::byte_view{ zeros16, pad_aad });

            if (ciphertext.len()) p.update(ciphertext);
            const usize pad_ct = pad16(ciphertext.len());
            if (pad_ct) p.update(a::byte_view{ zeros16, pad_ct });

            u8 lens[16]{};
            detail::store64_le(lens + 0, (u64)aad.len());
            detail::store64_le(lens + 8, (u64)ciphertext.len());
            p.update(a::as_view(lens));

            p.final(tag_out);
        }

        // NOTE: seal/open support in-place operation (plaintext may alias ciphertext_out).
        // Seal: encrypt plaintext -> ciphertext, compute tag.
        // ciphertext_out must be same size as plaintext.
        template<usize NAAD, usize NPT>
        static void seal(byte_view_n<KEY_BYTES>    key,
                         byte_view_n<NONCE_BYTES>  nonce,
                         byte_view_n<NAAD>         aad,
                         byte_view_n<NPT>          plaintext,
                         byte_view_mut_n<NPT>      ciphertext_out,
                         byte_view_mut_n<TAG_BYTES> tag_out) noexcept {
            // 1) Encrypt with counter=1
            a::crypto::chacha20 c;
            c.init(key, nonce, 1u);
            c.encrypt(plaintext, ciphertext_out);
            // 2) MAC over AAD/ciphertext per RFC 8439
            mac_rfc8439(key, nonce, aad, a::as_view(ciphertext_out), tag_out);
        }

        template<usize NAAD, usize NPT>
        static void seal(const u8(&key)[KEY_BYTES],
                         const u8(&nonce)[NONCE_BYTES],
                         const u8(&aad)[NAAD],
                         const u8(&plaintext)[NPT],
                         u8(&ciphertext_out)[NPT],
                         u8(&tag_out)[TAG_BYTES]) noexcept {
            seal(a::as_view(key), a::as_view(nonce),
                 a::as_view(aad), a::as_view(plaintext),
                 a::as_view_mut(ciphertext_out),
                 a::as_view_mut(tag_out));
        }

        // Open: verify tag, then decrypt ciphertext -> plaintext.
        // Returns true if tag is valid.
        template<usize NAAD, usize NCT>
        A_NODISCARD static bool open(byte_view_n<KEY_BYTES>   key,
                                      byte_view_n<NONCE_BYTES> nonce,
                                      byte_view_n<NAAD>        aad,
                                      byte_view_n<NCT>         ciphertext,
                                      byte_view_n<TAG_BYTES>   tag,
                                      byte_view_mut_n<NCT>     plaintext_out) noexcept {
            // 1) Compute expected tag
            u8 got[TAG_BYTES]{};
            mac_rfc8439(key, nonce, aad, ciphertext, a::as_view_mut(got));

            if (!a::crypto::poly1305::ct_equal(a::as_view(got), tag))
                return false;

            // 2) Decrypt with counter=1
            a::crypto::chacha20 c;
            c.init(key, nonce, 1u);
            c.encrypt(ciphertext, plaintext_out);
            return true;
        }

        template<usize NAAD, usize NCT>
        A_NODISCARD static bool open(const u8(&key)[KEY_BYTES],
                                      const u8(&nonce)[NONCE_BYTES],
                                      const u8(&aad)[NAAD],
                                      const u8(&ciphertext)[NCT],
                                      const u8(&tag)[TAG_BYTES],
                                      u8(&plaintext_out)[NCT]) noexcept {
            return open(a::as_view(key), a::as_view(nonce),
                        a::as_view(aad), a::as_view(ciphertext),
                        a::as_view(tag), a::as_view_mut(plaintext_out));
        }
    }; // struct aead_chacha20_poly1305
} // namespace crypto
} // namespace a
