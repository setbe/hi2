#include "thread.hpp"
#include "detail/port_contract.hpp"

namespace b {
#if B_HAS_THREADS
auto thread_current() noexcept -> thread_id {
    return detail::port::thread_current();
}

auto thread_yield() noexcept -> void {
    detail::port::thread_yield();
}
#endif
} // namespace b

