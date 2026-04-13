#include "../../detail/port_contract.hpp"

#if B_HAS_SOCKETS && B_OS_LINUX
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
A_NODISCARD A_FORCE_INLINE auto lsocket_valid(b::socket_handle h) noexcept -> bool {
    return h.native != static_cast<a::isize>(-1);
}

A_NODISCARD A_FORCE_INLINE auto to_native(b::socket_handle h) noexcept -> int {
    return static_cast<int>(h.native);
}

A_NODISCARD A_FORCE_INLINE auto from_native(int fd) noexcept -> b::socket_handle {
    return b::socket_handle{static_cast<a::isize>(fd)};
}

A_NODISCARD auto send_status_from_errno() noexcept -> b::socket_status {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        return b::socket_status::would_block;
    }
    if (errno == EBADF || errno == ENOTSOCK || errno == ECONNRESET) {
        return b::socket_status::closed;
    }
    return b::socket_status::failed;
}
}

namespace b {
namespace detail {
namespace port {
auto socket_open_udp() noexcept -> b::socket_handle {
    const int fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        return {};
    }

    const int fl = ::fcntl(fd, F_GETFL, 0);
    if (fl < 0 || ::fcntl(fd, F_SETFL, fl | O_NONBLOCK) != 0) {
        ::close(fd);
        return {};
    }
    return from_native(fd);
}

auto socket_close(b::socket_handle h) noexcept -> void {
    if (!lsocket_valid(h)) {
        return;
    }
    (void)::close(to_native(h));
}

auto socket_bind(b::socket_handle h, b::endpoint ep) noexcept -> bool {
    if (!lsocket_valid(h)) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ep.ipv4_be;
    addr.sin_port = ep.port_be;

    const int rc = ::bind(
        to_native(h),
        reinterpret_cast<const sockaddr*>(&addr),
        static_cast<socklen_t>(sizeof(addr))
    );
    return rc == 0;
}

auto socket_send_to(b::socket_handle h, b::endpoint ep, a::byte_view bytes) noexcept -> b::socket_status {
    if (!lsocket_valid(h) || (bytes.data() == nullptr && bytes.size() != 0)) {
        return b::socket_status::failed;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ep.ipv4_be;
    addr.sin_port = ep.port_be;

    const ssize_t rc = ::sendto(
        to_native(h),
        reinterpret_cast<const void*>(bytes.data()),
        static_cast<size_t>(bytes.size()),
        0,
        reinterpret_cast<const sockaddr*>(&addr),
        static_cast<socklen_t>(sizeof(addr))
    );
    if (rc >= 0) {
        return b::socket_status::ok;
    }
    return send_status_from_errno();
}

auto socket_recv_from(b::socket_handle h, a::byte_view_mut out) noexcept -> b::socket_recv_result {
    if (!lsocket_valid(h) || (out.data() == nullptr && out.size() != 0)) {
        return b::socket_recv_result{b::socket_status::failed, 0, b::endpoint{}};
    }

    sockaddr_in src{};
    socklen_t src_len = static_cast<socklen_t>(sizeof(src));
    const ssize_t rc = ::recvfrom(
        to_native(h),
        reinterpret_cast<void*>(out.data()),
        static_cast<size_t>(out.size()),
        0,
        reinterpret_cast<sockaddr*>(&src),
        &src_len
    );
    if (rc >= 0) {
        b::endpoint from{};
        from.ipv4_be = src.sin_addr.s_addr;
        from.port_be = src.sin_port;
        return b::socket_recv_result{b::socket_status::ok, static_cast<a::isize>(rc), from};
    }
    return b::socket_recv_result{send_status_from_errno(), 0, b::endpoint{}};
}
} // namespace port
} // namespace detail
} // namespace b
#endif

