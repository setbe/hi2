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
    a::isize native = static_cast<a::isize>(-1);
};

struct socket_recv_result {
    socket_status status = socket_status::failed;
    a::isize bytes = 0;
    endpoint from{};
};

#if B_HAS_SOCKETS
A_NODISCARD auto socket_open_udp() noexcept -> socket_handle;
auto socket_close(socket_handle h) noexcept -> void;
A_NODISCARD auto socket_bind(socket_handle h, endpoint ep) noexcept -> bool;
A_NODISCARD auto socket_send_to(socket_handle h, endpoint ep, a::byte_view bytes) noexcept -> socket_status;
A_NODISCARD auto socket_recv_from(socket_handle h, a::byte_view_mut out) noexcept -> socket_recv_result;

struct udp_socket final {
    udp_socket() noexcept;
    ~udp_socket() noexcept;

    udp_socket(const udp_socket&) = delete;
    auto operator=(const udp_socket&) -> udp_socket& = delete;

    udp_socket(udp_socket&& other) noexcept;
    auto operator=(udp_socket&& other) noexcept -> udp_socket&;

    A_NODISCARD auto valid() const noexcept -> bool;
    A_NODISCARD explicit operator bool() const noexcept;

    A_NODISCARD auto native_handle() const noexcept -> socket_handle;
    A_NODISCARD auto open() noexcept -> bool;
    auto close() noexcept -> void;

    A_NODISCARD auto bind(endpoint ep) noexcept -> bool;
    A_NODISCARD auto send_to(endpoint ep, a::byte_view bytes) noexcept -> socket_status;
    A_NODISCARD auto recv_from(a::byte_view_mut out) noexcept -> socket_recv_result;

    A_NODISCARD auto release() noexcept -> socket_handle;

private:
    socket_handle h_{};
};
#else
A_NODISCARD inline auto socket_open_udp() noexcept -> socket_handle = delete;
inline auto socket_close(socket_handle) noexcept -> void = delete;
A_NODISCARD inline auto socket_bind(socket_handle, endpoint) noexcept -> bool = delete;
A_NODISCARD inline auto socket_send_to(socket_handle, endpoint, a::byte_view) noexcept -> socket_status = delete;
A_NODISCARD inline auto socket_recv_from(socket_handle, a::byte_view_mut) noexcept -> socket_recv_result = delete;
#endif
} // namespace b

