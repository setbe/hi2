#pragma once

#include "../../a/types.hpp"
#include "../../a/view.hpp"
#include "../../b/capabilities.hpp"
#include "../../b/net/socket.hpp"

namespace c {
namespace net {
struct packet {
    a::byte_view bytes{};
};

struct session_id {
    a::u32 value = 0;
};

#if B_HAS_SOCKETS
A_NODISCARD auto send_reliable(b::socket_handle s, b::endpoint ep, session_id sid, packet p) noexcept -> bool;
A_NODISCARD auto recv_reliable(b::socket_handle s, b::endpoint* from, session_id* sid, a::byte_view_mut out) noexcept -> a::isize;
#else
A_NODISCARD inline auto send_reliable(b::socket_handle, b::endpoint, session_id, packet) noexcept -> bool = delete;
A_NODISCARD inline auto recv_reliable(b::socket_handle, b::endpoint*, session_id*, a::byte_view_mut) noexcept -> a::isize = delete;
#endif
} // namespace net
} // namespace c
