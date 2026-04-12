# b:: Porting Contract

`b::` is capability-based middleware.  
A platform implements only what it can provide.

## Rules

- `b` may depend only on `a`.
- Public headers in `b/` must not expose OS-native types in the API.
- Capability flags live in `b/capabilities.hpp`.
- Missing capability must fail at compile time via deleted API.

## Custom OS flow

1. Copy `b/ports/custom_os_template/` to `b/ports/<your_os>/`.
2. Set capability macros for your platform build.
3. Implement functions declared in `b/port_contract.hpp` for enabled capabilities.
4. Add your source files in build-system per capability module.

## Examples

- Pico-style target can expose: `threads + time`, but no filesystem/windowing.
- Desktop target can expose: `threads + time + sockets + filesystem + windowing`.

