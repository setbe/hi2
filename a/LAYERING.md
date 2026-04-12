# a Layering Invariant

`a/` is the stable freestanding core.

Dependency law inside `a/`:

- `config.hpp` -> no internal dependencies
- `detail/arch.hpp` -> `config.hpp`
- `types.hpp`       -> `config.hpp`
- `status.hpp`      -> `config.hpp`, `types.hpp`
- `memory_requirements.hpp` -> `config.hpp`, `types.hpp`
- `meta.hpp`        -> `config.hpp`, `types.hpp`
- `utility.hpp`     -> `config.hpp`, `meta.hpp`
- `arena.hpp`       -> `config.hpp`, `types.hpp`, `status.hpp`, `memory_requirements.hpp`
- `two_pass.hpp`    -> `types.hpp`, `status.hpp`, `memory_requirements.hpp`
- `math.hpp`        -> `config.hpp`, `types.hpp`, `detail/arch.hpp`
- `bit.hpp`         -> `config.hpp`, `types.hpp`
- `charconv_base.hpp` -> `config.hpp`, `types.hpp`
- `view.hpp`        -> `config.hpp`, `utility.hpp`
- `utf8.hpp`        -> `config.hpp`, `view.hpp`
- `crypto/detail/wipe.hpp` -> `types.hpp`
- `crypto/detail/load_store.hpp` -> `bit.hpp`, `types.hpp`
- `crypto/detail/ct.hpp` -> `view.hpp`
- `crypto/*` -> `a.hpp` + crypto-local headers, no OS dependencies

This order is considered a compatibility invariant for future refactors.
