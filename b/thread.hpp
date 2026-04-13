#pragma once

#include "../a/types.hpp"
#include "capabilities.hpp"

namespace b {
struct thread_id {
    a::u64 value = 0;
};

#if B_HAS_THREADS
A_NODISCARD auto thread_current() noexcept -> thread_id;
auto thread_yield() noexcept -> void;
#else
A_NODISCARD inline auto thread_current() noexcept -> thread_id = delete;
inline auto thread_yield() noexcept -> void = delete;
#endif
} // namespace b

