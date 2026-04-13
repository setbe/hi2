#include "socket.hpp"
#include "../detail/port_contract.hpp"

namespace {
A_NODISCARD A_FORCE_INLINE auto b_socket_valid(b::socket_handle h) noexcept -> bool {
    return h.native != static_cast<a::isize>(-1);
}
}

namespace b {
#if B_HAS_SOCKETS
auto socket_open_udp() noexcept -> socket_handle {
    return detail::port::socket_open_udp();
}

auto socket_close(socket_handle h) noexcept -> void {
    detail::port::socket_close(h);
}

auto socket_bind(socket_handle h, endpoint ep) noexcept -> bool {
    return detail::port::socket_bind(h, ep);
}

auto socket_send_to(socket_handle h, endpoint ep, a::byte_view bytes) noexcept -> socket_status {
    return detail::port::socket_send_to(h, ep, bytes);
}

auto socket_recv_from(socket_handle h, a::byte_view_mut out) noexcept -> socket_recv_result {
    return detail::port::socket_recv_from(h, out);
}

udp_socket::udp_socket() noexcept = default;
udp_socket::~udp_socket() noexcept { close(); }

udp_socket::udp_socket(udp_socket&& other) noexcept : h_(other.release()) {}

auto udp_socket::operator=(udp_socket&& other) noexcept -> udp_socket& {
    if (this != &other) {
        close();
        h_ = other.release();
    }
    return *this;
}

auto udp_socket::valid() const noexcept -> bool {
    return b_socket_valid(h_);
}

udp_socket::operator bool() const noexcept {
    return valid();
}

auto udp_socket::native_handle() const noexcept -> socket_handle {
    return h_;
}

auto udp_socket::open() noexcept -> bool {
    close();
    h_ = socket_open_udp();
    return valid();
}

auto udp_socket::close() noexcept -> void {
    if (!valid()) {
        return;
    }
    socket_close(h_);
    h_ = socket_handle{};
}

auto udp_socket::bind(endpoint ep) noexcept -> bool {
    return valid() && socket_bind(h_, ep);
}

auto udp_socket::send_to(endpoint ep, a::byte_view bytes) noexcept -> socket_status {
    if (!valid()) {
        return socket_status::closed;
    }
    return socket_send_to(h_, ep, bytes);
}

auto udp_socket::recv_from(a::byte_view_mut out) noexcept -> socket_recv_result {
    if (!valid()) {
        return socket_recv_result{socket_status::closed, 0, endpoint{}};
    }
    return socket_recv_from(h_, out);
}

auto udp_socket::release() noexcept -> socket_handle {
    socket_handle out = h_;
    h_ = socket_handle{};
    return out;
}
#endif
} // namespace b

