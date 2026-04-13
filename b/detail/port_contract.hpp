#pragma once

#include "../../a/types.hpp"
#include "../../a/view.hpp"
#include "../capabilities.hpp"
#include "../thread.hpp"
#include "../time.hpp"
#include "../net/socket.hpp"

namespace b {
namespace detail {
namespace port {
#if B_HAS_THREADS
A_NODISCARD auto thread_current() noexcept -> b::thread_id;
auto thread_yield() noexcept -> void;
#endif

#if B_HAS_TIME
A_NODISCARD auto monotonic_now_ns() noexcept -> a::u64;
auto sleep_ms(a::u32 ms) noexcept -> void;
#endif

#if B_HAS_SOCKETS
A_NODISCARD auto socket_open_udp() noexcept -> b::socket_handle;
auto socket_close(b::socket_handle h) noexcept -> void;
A_NODISCARD auto socket_bind(b::socket_handle h, b::endpoint ep) noexcept -> bool;
A_NODISCARD auto socket_send_to(b::socket_handle h, b::endpoint ep, a::byte_view bytes) noexcept -> b::socket_status;
A_NODISCARD auto socket_recv_from(b::socket_handle h, a::byte_view_mut out) noexcept -> b::socket_recv_result;
#endif
} // namespace port
} // namespace detail
} // namespace b

