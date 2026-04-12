#pragma once

#include "../a/types.hpp"
#include "capabilities.hpp"

namespace b {
struct thread_id {
    a::u32 value = 0;
};

#if B_HAS_THREADS
A_NODISCARD auto thread_current() noexcept -> thread_id;
auto thread_sleep_ms(a::u32 ms) noexcept -> void;
auto thread_yield() noexcept -> void;
#else
A_NODISCARD inline auto thread_current() noexcept -> thread_id = delete;
inline auto thread_sleep_ms(a::u32) noexcept -> void = delete;
inline auto thread_yield() noexcept -> void = delete;
#endif
} // namespace b

