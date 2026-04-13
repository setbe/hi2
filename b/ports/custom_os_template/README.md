# custom_os_template

Implement only enabled capabilities from `b/detail/port_contract.hpp`.

Suggested split:

- `thread.cpp` for thread APIs
- `time.cpp` for clock/sleep
- `socket.cpp` for UDP socket APIs

If a capability is unavailable, keep `B_HAS_*` as `0` and do not implement it.
