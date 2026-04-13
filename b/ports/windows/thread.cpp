#include "../../detail/port_contract.hpp"

#if B_HAS_THREADS && B_OS_WINDOWS
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
auto thread_current() noexcept -> b::thread_id {
    return b::thread_id{static_cast<a::u64>(::GetCurrentThreadId())};
}

auto thread_yield() noexcept -> void {
    ::SwitchToThread();
}
} // namespace port
} // namespace detail
} // namespace b
#endif

