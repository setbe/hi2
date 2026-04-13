# b

Freestanding OS/environment middleware (`namespace b::`).

`b` depends only on `a`.

Capability-based API:

- unavailable capability => API is compile-time deleted
- custom OS can implement only supported subset
- public headers stay OS-neutral (no platform includes)
- platform implementations live in `b/ports/<platform>/*.cpp`

Start here:

- `capabilities.hpp`
- `detail/port_contract.hpp`
- `PORTING.md`
