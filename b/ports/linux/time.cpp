#include "../../detail/port_contract.hpp"

#if B_HAS_TIME && B_OS_LINUX
#include <errno.h>
#include <time.h>

namespace b {
namespace detail {
namespace port {
auto monotonic_now_ns() noexcept -> a::u64 {
    timespec ts{};
    if (::clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    if (ts.tv_sec < 0 || ts.tv_nsec < 0) {
        return 0;
    }
    return static_cast<a::u64>(ts.tv_sec) * 1000000000ull + static_cast<a::u64>(ts.tv_nsec);
}

auto sleep_ms(a::u32 ms) noexcept -> void {
    timespec req{};
    req.tv_sec = static_cast<time_t>(ms / 1000u);
    req.tv_nsec = static_cast<long>((ms % 1000u) * 1000000u);
    while (::nanosleep(&req, &req) != 0 && errno == EINTR) {}
}
} // namespace port
} // namespace detail
} // namespace b
#endif

