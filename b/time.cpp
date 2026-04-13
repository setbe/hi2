#include "time.hpp"
#include "detail/port_contract.hpp"

namespace b {
#if B_HAS_TIME
auto monotonic_now_ns() noexcept -> a::u64 {
    return detail::port::monotonic_now_ns();
}

auto monotonic_ms() noexcept -> a::u64 {
    return monotonic_now_ns() / 1000000ull;
}

auto monotonic_us() noexcept -> a::u64 {
    return monotonic_now_ns() / 1000ull;
}

auto sleep_ms(a::u32 ms) noexcept -> void {
    detail::port::sleep_ms(ms);
}
#endif
} // namespace b

