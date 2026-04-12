# ABCD Migration (Phase 1.1)

This repository now has first-class root directories:
- `a/` bare-metal freestanding core
- `b/` freestanding OS/environment middleware
- `c/` freestanding high-level portable layer
- `d/` planned higher-level layer

## Dependency Law

- `a ->` no dependencies on `b/c/d`
- `b -> a`
- `c -> a + b`
- `d -> a + b + c`

## What Was Done In Phase 1.1

- Added core `a::` headers:
  - `a/config.hpp`
  - `a/types.hpp`
  - `a/meta.hpp`
  - `a/utility.hpp`
  - `a/math.hpp`
  - `a/view.hpp`
  - `a/utf8.hpp`
  - `a/a.hpp`
- Added `b::` capability contract:
  - `b/capabilities.hpp`
  - `b/thread.hpp`
  - `b/time.hpp`
  - `b/net/socket.hpp`
  - `b/port_contract.hpp`
  - `b/PORTING.md`
- Added initial `c::` and `d::` shells:
  - `c/app/window.hpp`
  - `c/net/reliable_udp.hpp`
  - `c/c.hpp`
  - `d/d.hpp`
- Legacy `hi/*` stays intact for incremental migration.

## Next Step (Phase 2)

- Wire `b/detail::port` implementations per platform:
  - `b/ports/win32/*`
  - `b/ports/linux/*`
  - `b/ports/pico/*`
  - `b/ports/<custom_os>/*`
- Move OS code from `hi/io.hpp` into `b/` modules.
- Move high-level API pieces from `hi.hpp`, `gl_loader.hpp`, `socket.hpp`, `audiocontext.hpp` into `c/`.
- Keep compatibility shims (`io:: -> a::/b::/c::`) until all examples/tests stop including `hi/*`.

## Legacy File Split Plan

- `hi/io.hpp`
  - `a/*` for core primitives (types/meta/view/math/utf8/memory)
  - `b/thread.hpp`, `b/time.hpp`, `b/fs/*`, `b/net/*` for OS wrappers
- `hi/socket.hpp`
  - `b/net/socket.hpp` for raw socket ABI
  - `c/net/reliable_udp.hpp` for protocol/session logic
- `hi/gl_loader.hpp`
  - `b/gfx/gl_api.hpp` for raw proc loading
  - `c/gfx/gl.hpp` for typed wrappers and helpers
- `hi/hi.hpp`
  - `b/window/*` for native handles/backends
  - `c/app/window.hpp` and `c/input/*` for portable app/events contracts
- `hi/audiocontext.hpp`
  - `b/audio/device.*` for OS mixer/device backend
  - `c/audio/*` for scheduler/music/game-level logic
