#include "../../detail/port_contract.hpp"

#if B_HAS_THREADS && B_OS_LINUX
#include <pthread.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace {
A_NODISCARD auto pthread_token() noexcept -> a::u64 {
    const pthread_t self = ::pthread_self();
    a::u64 out = 0;
    const unsigned char* src = reinterpret_cast<const unsigned char*>(&self);
    unsigned char* dst = reinterpret_cast<unsigned char*>(&out);
    const a::usize n = (sizeof(pthread_t) < sizeof(out)) ? sizeof(pthread_t) : sizeof(out);
    for (a::usize i = 0; i < n; ++i) {
        dst[i] = src[i];
    }
    if (out == 0) {
        out = 1;
    }
    return out;
}
}

namespace b {
namespace detail {
namespace port {
auto thread_current() noexcept -> b::thread_id {
#if defined(SYS_gettid)
    const long tid = ::syscall(SYS_gettid);
    if (tid > 0) {
        return b::thread_id{static_cast<a::u64>(tid)};
    }
#endif
    // Opaque fallback token when native gettid is unavailable.
    return b::thread_id{pthread_token()};
}

auto thread_yield() noexcept -> void {
    (void)::sched_yield();
}
} // namespace port
} // namespace detail
} // namespace b
#endif

