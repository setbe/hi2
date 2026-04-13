#pragma once

#include "../a/types.hpp"
#include "capabilities.hpp"

namespace b {
#if B_HAS_TIME
A_NODISCARD auto monotonic_now_ns() noexcept -> a::u64;
A_NODISCARD auto monotonic_ms() noexcept -> a::u64;
A_NODISCARD auto monotonic_us() noexcept -> a::u64;
auto sleep_ms(a::u32 ms) noexcept -> void;
#else
A_NODISCARD inline auto monotonic_now_ns() noexcept -> a::u64 = delete;
A_NODISCARD inline auto monotonic_ms() noexcept -> a::u64 = delete;
A_NODISCARD inline auto monotonic_us() noexcept -> a::u64 = delete;
inline auto sleep_ms(a::u32) noexcept -> void = delete;
#endif
} // namespace b

