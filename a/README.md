# a

Freestanding bare-metal core (`namespace a::`).

No OS headers. No syscalls. No windowing/sockets/filesystem assumptions.

Current modules:

- `config.hpp`
- `profile.hpp` (contract profile presets: strict/audit/lean)
- `contract.hpp` (`require/ensure/invariant/assume/assume_verified/unreachable`)
- `bounded.hpp` (`bounded`, `pow2_bounded`, `static_bounded`)
- `detail/arch.hpp` (internal tags/policy selection)
- `types.hpp`
- `status.hpp`
- `memory_requirements.hpp`
- `meta.hpp`
- `utility.hpp`
- `arena.hpp` (`arena_view`, `scope`)
- `two_pass.hpp`
  - `a::component_req<T>`
  - `a::sum_req<Ts...>` / `a::max_req<Ts...>`
  - `a::layout<Ts...>` / `a::scope_spec<Ts...>`
  - `a::typed_scope<Layout>` (`slot<T>()` raw storage, `construct/destroy`, `get<T>()` constructed-only, `active()` for usability, `init_status()` for init result)
- `math.hpp`
- `bit.hpp`
- `charconv_base.hpp`
- `view.hpp`
- `utf8.hpp`
- `crypto/chacha20.hpp`
- `crypto/poly1305.hpp`
- `crypto/aead_chacha20_poly1305.hpp`
- `crypto/sha256.hpp`
- `crypto/sha384.hpp`
- `crypto/x25519.hpp`
- `crypto/detail/load_store.hpp`
- `crypto/detail/ct.hpp`
- `crypto/detail/wipe.hpp`
- `a.hpp` (umbrella include)
- `CONTRACTS.md` (short contract usage rules)

