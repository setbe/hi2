#include "../../detail/port_contract.hpp"

#if B_HAS_TIME && B_OS_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace b {
namespace detail {
namespace port {
auto monotonic_now_ns() noexcept -> a::u64 {
    static const a::u64 freq = []() -> a::u64 {
        LARGE_INTEGER f{};
        if (::QueryPerformanceFrequency(&f) == 0 || f.QuadPart <= 0) {
            return 0;
        }
        return static_cast<a::u64>(f.QuadPart);
    }();
    if (freq == 0) {
        return 0;
    }

    LARGE_INTEGER c{};
    if (::QueryPerformanceCounter(&c) == 0 || c.QuadPart < 0) {
        return 0;
    }
    const a::u64 ticks = static_cast<a::u64>(c.QuadPart);
    const a::u64 whole = (ticks / freq) * 1000000000ull;
    const a::u64 frac = ((ticks % freq) * 1000000000ull) / freq;
    return whole + frac;
}

auto sleep_ms(a::u32 ms) noexcept -> void {
    ::Sleep(static_cast<DWORD>(ms));
}
} // namespace port
} // namespace detail
} // namespace b
#endif

