#include "../../detail/port_contract.hpp"

#if B_HAS_SOCKETS && B_OS_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#if defined(_MSC_VER)
#pragma comment(lib, "Ws2_32.lib")
#endif

namespace {
A_NODISCARD A_FORCE_INLINE auto wsocket_valid(b::socket_handle h) noexcept -> bool {
    return h.native != static_cast<a::isize>(-1);
}

A_NODISCARD A_FORCE_INLINE auto to_native(b::socket_handle h) noexcept -> SOCKET {
    return static_cast<SOCKET>(static_cast<UINT_PTR>(h.native));
}

A_NODISCARD A_FORCE_INLINE auto from_native(SOCKET s) noexcept -> b::socket_handle {
    return b::socket_handle{static_cast<a::isize>(static_cast<UINT_PTR>(s))};
}

A_NODISCARD auto socket_startup() noexcept -> bool {
    static const bool ok = []() -> bool {
        WSADATA data{};
        return ::WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    return ok;
}

A_NODISCARD auto last_send_status() noexcept -> b::socket_status {
    const int e = ::WSAGetLastError();
    if (e == WSAEWOULDBLOCK || e == WSAEINTR) {
        return b::socket_status::would_block;
    }
    if (e == WSAENOTSOCK || e == WSAENOTCONN || e == WSAECONNRESET) {
        return b::socket_status::closed;
    }
    return b::socket_status::failed;
}
}

namespace b {
namespace detail {
namespace port {
auto socket_open_udp() noexcept -> b::socket_handle {
    if (!socket_startup()) {
        return {};
    }

    SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        return {};
    }

    u_long mode = 1;
    if (::ioctlsocket(s, FIONBIO, &mode) != 0) {
        ::closesocket(s);
        return {};
    }
    return from_native(s);
}

auto socket_close(b::socket_handle h) noexcept -> void {
    if (!wsocket_valid(h)) {
        return;
    }
    (void)::closesocket(to_native(h));
}

auto socket_bind(b::socket_handle h, b::endpoint ep) noexcept -> bool {
    if (!wsocket_valid(h)) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ep.ipv4_be;
    addr.sin_port = ep.port_be;

    const int rc = ::bind(
        to_native(h),
        reinterpret_cast<const sockaddr*>(&addr),
        static_cast<int>(sizeof(addr))
    );
    return rc == 0;
}

auto socket_send_to(b::socket_handle h, b::endpoint ep, a::byte_view bytes) noexcept -> b::socket_status {
    if (!wsocket_valid(h) || (bytes.data() == nullptr && bytes.size() != 0)) {
        return b::socket_status::failed;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ep.ipv4_be;
    addr.sin_port = ep.port_be;

    const int rc = ::sendto(
        to_native(h),
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<int>(bytes.size()),
        0,
        reinterpret_cast<const sockaddr*>(&addr),
        static_cast<int>(sizeof(addr))
    );
    if (rc >= 0) {
        return b::socket_status::ok;
    }
    return last_send_status();
}

auto socket_recv_from(b::socket_handle h, a::byte_view_mut out) noexcept -> b::socket_recv_result {
    if (!wsocket_valid(h) || (out.data() == nullptr && out.size() != 0)) {
        return b::socket_recv_result{b::socket_status::failed, 0, b::endpoint{}};
    }

    sockaddr_in src{};
    int src_len = static_cast<int>(sizeof(src));
    const int rc = ::recvfrom(
        to_native(h),
        reinterpret_cast<char*>(out.data()),
        static_cast<int>(out.size()),
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

    const b::socket_status st = last_send_status();
    return b::socket_recv_result{st, 0, b::endpoint{}};
}
} // namespace port
} // namespace detail
} // namespace b
#endif

