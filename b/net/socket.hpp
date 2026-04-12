#pragma once

#include "../../a/types.hpp"
#include "../../a/view.hpp"
#include "../capabilities.hpp"

namespace b {
struct endpoint {
    a::u32 ipv4_be = 0;
    a::u16 port_be = 0;
};

enum class socket_status : a::u8 {
    ok = 0,
    would_block = 1,
    closed = 2,
    failed = 3
};

struct socket_handle {
    a::i32 native = -1;
};

#if B_HAS_SOCKETS
A_NODISCARD auto socket_open_udp() noexcept -> socket_handle;
auto socket_close(socket_handle h) noexcept -> void;
A_NODISCARD auto socket_bind(socket_handle h, endpoint ep) noexcept -> bool;
A_NODISCARD auto socket_send_to(socket_handle h, endpoint ep, a::byte_view bytes) noexcept -> socket_status;
A_NODISCARD auto socket_recv_from(socket_handle h, endpoint* from, a::byte_view_mut out) noexcept -> a::isize;
#else
A_NODISCARD inline auto socket_open_udp() noexcept -> socket_handle = delete;
inline auto socket_close(socket_handle) noexcept -> void = delete;
A_NODISCARD inline auto socket_bind(socket_handle, endpoint) noexcept -> bool = delete;
A_NODISCARD inline auto socket_send_to(socket_handle, endpoint, a::byte_view) noexcept -> socket_status = delete;
A_NODISCARD inline auto socket_recv_from(socket_handle, endpoint*, a::byte_view_mut) noexcept -> a::isize = delete;
#endif
} // namespace b

