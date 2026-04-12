/*
    Minimal UDP Netcode (Freestanding C++)

    Current state (implemented, working):
    - Non-blocking UDP event loop (WSAPoll / epoll style).
    - Client/server handshake:
        HELLO -> (optional COOKIE challenge) -> HELLO2 -> WELCOME.
    - Anti-replay + anti-spoof:
        server cookies + per-session keys + packet counters / nonces.
    - Feature flags + MTU negotiation in handshake.
    - Peer table with session IDs and disconnect reasons.
    - Reliable channel support (ack/ack_bits + retransmit).
    - Security-oriented tests:
        - Garbage / malformed packets must not elicit COOKIE/WELCOME.
        - Peer-table pressure: server must still accept honest clients after spam.

    TODO:
    - Explicit timeout model (not compile-time constants):
        handshake_timeout_ms, peer_idle_timeout_ms, keepalive_interval_ms,
        reliable_rto_ms (+ backoff).
    - Session/epoch guard:
        User packets must be rejected if they belong to an old session.

    Next milestone: encrypted transport:
    - AEAD: ChaCha20 for all user packets.
    - Key exchange: X25519 (elliptic curve Diffie-Hellman).
    - Public key distribution options:
        1) Pinned server public key embedded into the client (simple, robust).
        2) PKI-based approach:
           Use a CA-signed certificate (e.g., Let's Encrypt) and verify it,
           then extract/validate the server key during the initial connection.

    Goal:
    Provide a small, freestanding, test-driven UDP networking core suitable
    for real-time multiplayer games (Minecraft-like, action, etc.).
*/


#pragma once
#include "io.hpp"
#include "crypto/poly1305.hpp"

#ifndef assert
#   ifdef _DEBUG
#       include <assert.h>
#   else
#       define assert
#   endif
#endif

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   pragma comment(lib, "Ws2_32.lib")
#elif defined(__linux__)
    // nothing to include
#else
#   error "OS isn't specified"
#endif

namespace io {
    static IO_CONSTEXPR_VAR u32 UDP_MAGIC = 0x48494F55u; // 'UIOH'
    static IO_CONSTEXPR_VAR u16 UDP_VERSION = 1;
    static IO_CONSTEXPR_VAR u32 FEATURE_COOKIE = 1u; // supports cookie handshake

    // MTU contract: payload cap we promise to never exceed after handshake.
    static IO_CONSTEXPR_VAR u16 DEFAULT_MTU = 1200;
    static IO_CONSTEXPR_VAR u16 MIN_MTU = 512;
    static IO_CONSTEXPR_VAR u16 MAX_MTU = 1400; // safe-ish for internet UDP (no fragmentation)

    // session timeouts
    static IO_CONSTEXPR_VAR u32 HANDSHAKE_TIMEOUT_MS = 3000;
    static IO_CONSTEXPR_VAR u32 PEER_TIMEOUT_MS = 5000;
    static IO_CONSTEXPR_VAR u32 KEEPALIVE_INTERVAL_MS = 1000;

    static IO_CONSTEXPR_VAR usize MAX_PEERS = 64;
    static IO_CONSTEXPR_VAR usize REL_INFLIGHT = 64; // pick sane inflight cap; 64..256 depends on your needs

    enum : u8 {
        MSG_HELLO      = 1,
        MSG_COOKIE     = 2, // server -> client challenge
        MSG_WELCOME    = 3,
        MSG_DISCONNECT = 4,
        MSG_KEEPALIVE  = 5,
        MSG_HELLO2     = 6,  // client -> server, includes cookie
        MSG_KEEPALIVE2 = 7,  // keepalive payload (optional)
    };

    enum class DropReason : u8 {
        TooSmall = 1,
        BadMagic = 2,
        BadVer   = 3,
        BadLen   = 4,
        BadHs    = 5,
        BadCtrl  = 6,
        BadMtu   = 7,
        FullPeerTable = 8,
    };

    static const char* drop_reason_str(DropReason r) {
        switch (r) {
            case DropReason::TooSmall: return "TooSmall";
            case DropReason::BadMagic: return "BadMagic";
            case DropReason::BadVer:   return "BadVer";
            case DropReason::BadLen:   return "BadLen";
            case DropReason::BadHs:    return "BadHs";
            case DropReason::BadCtrl:  return "BadCtrl";
            case DropReason::BadMtu:   return "BadMtu";
            case DropReason::FullPeerTable:   return "FullPeerTable";
            default: return "?";
        }
    }


    // payloads (packed)
#pragma pack(push, 1)
    struct msg_hello {
        u16 mtu;
        u16 features;
        u32 client_nonce;
    };
    struct msg_cookie {
        u8 cookie[16];
        u32 nonce_echo;
    };
    struct msg_welcome {
        u16 mtu;
        u16 features;
        u32 session_id;
    };
    struct msg_disconnect {
        u32 session_id;  // network order
        u8  reason;      // DisconnectReason
        u8 _pad[3]{};
    };
    struct msg_hello2 {
        u8 cookie[16];
        u32 client_nonce;
        u16 mtu;
        u16 features;
    };
    struct msg_keepalive2 {
        u32 session_id;
    };

    struct poly_cookie {
        u32 ip_be;
        u32 nonce_be;
        u32 time_bucket_be; // e.g. now_ms >> 10
        u16 port_be;
        u16 ver_be;         // UDP_VERSION (optional)
    };
#pragma pack(pop)
    static_assert(sizeof(msg_hello) == 8, "msg_hello layout");
    static_assert(sizeof(msg_cookie) == 16+4, "msg_cookie layout");
    static_assert(sizeof(msg_welcome) == 8, "msg_welcome layout");
    static_assert(sizeof(msg_disconnect) == 8, "msg_disconnet layout");
    static_assert(sizeof(msg_hello2) == 16+4+2+2, "msg_hello2 layout");
    static_assert(sizeof(msg_keepalive2) == 4, "msg_keepalive2 layout");
    static_assert(sizeof(poly_cookie) == 4+4+4+2+2, "");





#ifdef __linux__
namespace native {
// -------- socket ABI constants --------
    static IO_CONSTEXPR_VAR int k_af_inet     = 2;
    static IO_CONSTEXPR_VAR int k_sock_stream = 1;
    static IO_CONSTEXPR_VAR int k_sock_dgram  = 2;

    static IO_CONSTEXPR_VAR int k_ipproto_udp = 17;
    static IO_CONSTEXPR_VAR int k_ipproto_tcp = 6;

    // fcntl
    static IO_CONSTEXPR_VAR int k_f_getfl   = 3;
    static IO_CONSTEXPR_VAR int k_f_setfl   = 4;
    static IO_CONSTEXPR_VAR int k_o_nonblock= 0x800; // Linux O_NONBLOCK ABI

    // -------- sockaddr ABI --------
    using socklen_t = unsigned int;

    struct in_addr { u32 s_addr; };

    struct sockaddr_in {
        u16 sin_family;
        u16 sin_port;
        in_addr sin_addr;
        unsigned char sin_zero[8];
    };

    struct sockaddr {
        unsigned short sa_family;
        char sa_data[14];
    };

    // -------- epoll ABI --------
    static IO_CONSTEXPR_VAR int k_epollin       = 0x001;
    static IO_CONSTEXPR_VAR int k_epoll_ctl_add = 1;

    union epoll_data {
        void* ptr;
        int   fd;
        u32   u32_;
        u64   u64_;
    };

    struct epoll_event {
        u32 events;
        epoll_data data;
    };

#if defined(__x86_64__)
    static IO_CONSTEXPR_VAR long k_sys_socket       = 41;
    static IO_CONSTEXPR_VAR long k_sys_bind         = 49;
    static IO_CONSTEXPR_VAR long k_sys_sendto       = 44;
    static IO_CONSTEXPR_VAR long k_sys_recvfrom     = 45;
    static IO_CONSTEXPR_VAR long k_sys_fcntl        = 72;

    static IO_CONSTEXPR_VAR long k_sys_epoll_create1= 291;
    static IO_CONSTEXPR_VAR long k_sys_epoll_ctl    = 233;
    static IO_CONSTEXPR_VAR long k_sys_epoll_wait   = 232;

#elif defined(__i386__)
    static IO_CONSTEXPR_VAR long k_sys_socketcall   = 102; // i386 uses socketcall multiplexer
    static IO_CONSTEXPR_VAR long k_sys_fcntl        = 55;

    static IO_CONSTEXPR_VAR long k_sys_epoll_create1= 329;
    static IO_CONSTEXPR_VAR long k_sys_epoll_ctl    = 255;
    static IO_CONSTEXPR_VAR long k_sys_epoll_wait   = 256;

    // socketcall ops:
    static IO_CONSTEXPR_VAR long k_sc_socket   = 1;
    static IO_CONSTEXPR_VAR long k_sc_bind     = 2;
    static IO_CONSTEXPR_VAR long k_sc_sendto   = 11;
    static IO_CONSTEXPR_VAR long k_sc_recvfrom = 12;
#else
#   error "linux: unsupported arch"
#endif

// --- helpers ---
#if defined(__i386__)
    static inline long socketcall(long op, long* args) noexcept {
        return sys2(k_sys_socketcall, op, (long)(usize)args);
    }
#endif

    static inline long sys_socket(int domain, int type, int protocol) noexcept {
#if defined(__x86_64__)
        return sys3(k_sys_socket, domain, type, protocol);
#else
        long a[3]{ domain, type, protocol };
        return socketcall(k_sc_socket, a);
#endif
    }

    static inline long sys_bind(int fd, const void* addr, unsigned len) noexcept {
#if defined(__x86_64__)
        return sys3(k_sys_bind, fd, (long)(usize)addr, (long)len);
#else
        long a[3]{ fd, (long)(usize)addr, (long)len };
        return socketcall(k_sc_bind, a);
#endif
    }

    static inline long sys_sendto(int fd, const void* buf, unsigned long len, int flags,
                                  const void* addr, unsigned addrlen) noexcept {
#if defined(__x86_64__)
        return sys6(k_sys_sendto, fd, (long)(usize)buf, (long)len, flags, (long)(usize)addr, (long)addrlen);
#else
        long a[6]{ fd, (long)(usize)buf, (long)len, flags, (long)(usize)addr, (long)addrlen };
        return socketcall(k_sc_sendto, a);
#endif
    }

    static inline long sys_recvfrom(int fd, void* buf, unsigned long len, int flags,
                                    void* addr, unsigned* addrlen_inout) noexcept {
#if defined(__x86_64__)
        return sys6(k_sys_recvfrom, fd, (long)(usize)buf, (long)len, flags, (long)(usize)addr, (long)(usize)addrlen_inout);
#else
        long a[6]{ fd, (long)(usize)buf, (long)len, flags, (long)(usize)addr, (long)(usize)addrlen_inout };
        return socketcall(k_sc_recvfrom, a);
#endif
    }
    static inline long sys_close(int fd) noexcept { return sys1(k_sys_close, fd); }
    static inline long sys_fcntl(int fd, int cmd, long arg) noexcept { return sys3(k_sys_fcntl, fd, cmd, arg); }
    static inline long sys_epoll_create1(int flags) noexcept { return sys1(k_sys_epoll_create1, flags); }
    static inline long sys_epoll_ctl(int epfd, int op, int fd, epoll_event* ev) noexcept { return sys4(k_sys_epoll_ctl, epfd, op, fd, (long)(usize)ev); }
    static inline long sys_epoll_wait(int epfd, epoll_event* evs, int maxevents, int timeout_ms) noexcept { return sys4(k_sys_epoll_wait, epfd, (long)(usize)evs, maxevents, timeout_ms); }
} // namespace native
#endif // __linux__

    // ========================= byte order (endian-safe) =========================
    // Assumptions: protocol is defined in network byte order (big-endian).
    // We implement bswap and use it conditionally for big-endian hosts.

    static IO_CONSTEXPR u16 bswap16(u16 x) noexcept { return (u16)((x << 8) | (x >> 8)); }
    static IO_CONSTEXPR u32 bswap32(u32 x) noexcept {
        return ((x & 0x000000FFu) << 24) |
            ((x & 0x0000FF00u) << 8) |
            ((x & 0x00FF0000u) >> 8) |
            ((x & 0xFF000000u) >> 24);
    }
    static IO_CONSTEXPR u64 bswap64(u64 x) noexcept {
        return ((x & 0x00000000000000FFull) << 56) |
            ((x & 0x000000000000FF00ull) << 40) |
            ((x & 0x0000000000FF0000ull) << 24) |
            ((x & 0x00000000FF000000ull) << 8) |
            ((x & 0x000000FF00000000ull) >> 8) |
            ((x & 0x0000FF0000000000ull) >> 24) |
            ((x & 0x00FF000000000000ull) >> 40) |
            ((x & 0xFF00000000000000ull) >> 56);
    }

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#   define IO_LITTLE_ENDIAN 1
#elif defined(_WIN32)
#   define IO_LITTLE_ENDIAN 1
#else
#   define IO_LITTLE_ENDIAN 0
#endif

    IO_CONSTEXPR u16 h2ns(u16 x) noexcept { return IO_LITTLE_ENDIAN ? bswap16(x) : x; }
    IO_CONSTEXPR u32 h2nl(u32 x) noexcept { return IO_LITTLE_ENDIAN ? bswap32(x) : x; }
    IO_CONSTEXPR u64 h2nll(u64 x) noexcept { return IO_LITTLE_ENDIAN ? bswap64(x) : x; }
    IO_CONSTEXPR u16 n2hs(u16 x) noexcept  { return ::io::h2ns(x); }
    IO_CONSTEXPR u32 n2hl(u32 x) noexcept  { return ::io::h2nl(x); }
    IO_CONSTEXPR u64 n2hll(u64 x) noexcept { return ::io::h2nll(x); }

    static IO_CONSTEXPR u32 parse_component(char_view s, usize& i) noexcept {
        const usize n = s.size();
        u32 v = 0;
        int digits = 0;

        while (i < n) {
            const char c = s[i];
            if (c < '0' || c > '9') break;

            v = v * 10u + (u32)(c - '0');
            ++i;
            ++digits;

            if (digits > 3) return 256; // invalid marker
        }
        return v;
    }

    // ------------------------- ipv4 -------------------------------------
    struct IP {
        u32 addr_be; // network byte order explicit
        IO_CONSTEXPR IP(u32 be = 0) noexcept : addr_be(be) {}

        // "X.Y.Z.W" -> u32 (network order), returns 0 on failure
        static IO_CONSTEXPR u32 from_string(char_view s) noexcept {
            if (!s || s.size() < 7) return 0;
            const usize n = s.size(); usize i = 0;
            const u32 o0 = parse_component(s, i); if (o0>255 || i>=n || s[i]!='.') return 0; ++i;
            const u32 o1 = parse_component(s, i); if (o1>255 || i>=n || s[i]!='.') return 0; ++i;
            const u32 o2 = parse_component(s, i); if (o2>255 || i>=n || s[i]!='.') return 0; ++i;
            const u32 o3 = parse_component(s, i); if (o3>255 || i!=n) return 0;
            u32 host = ((o0 & 0xFFu) << 24) | ((o1 & 0xFFu) << 16) | ((o2 & 0xFFu) << 8) | (o3 & 0xFFu);
            return ::io::h2nl(host);
        }
    }; // struct IP

    // ------------------------- endpoint --------------------------------
    struct Endpoint {
        u32 addr_be{ 0 };
        u16 port_be{ 0 };
        IO_CONSTEXPR Endpoint() noexcept = default;
        IO_CONSTEXPR Endpoint(u32 a_be, u16 p_be) noexcept : addr_be(a_be), port_be(p_be) {}
    };
    static inline bool endpoint_eq(const Endpoint& a, const Endpoint& b) noexcept {
        return a.addr_be == b.addr_be && a.port_be == b.port_be;
    }
    static IO_CONSTEXPR u16 clamp_mtu(u16 v) noexcept {
        if (v < MIN_MTU) return MIN_MTU;
        if (v > MAX_MTU) return MAX_MTU;
        return v;
    }
    static inline void cookie_tag16(byte_view_mut_n<16> out16, byte_view_n<32> key32,
                                    Endpoint ep, u32 nonce, u64 now_ms) noexcept {
        poly_cookie pc{};
        pc.ip_be          = ep.addr_be;
        pc.nonce_be       = ::io::h2nl(nonce);
        pc.time_bucket_be = ::io::h2nl((u32)(now_ms >> 10)); // div 1024
        pc.port_be        = ep.port_be;
        pc.ver_be         = ::io::h2ns(UDP_VERSION);

        cl::poly1305 mac;
        mac.init(key32);
        mac.update(byte_view_n<sizeof(pc)>{ (const io::u8*)&pc });
        mac.final(out16);
    }
    static inline bool cookie_check(byte_view_n<16> recv16, byte_view_n<32> key32,
                                    Endpoint ep, io::u32 nonce, io::u64 now_ms) noexcept {
        io::u8 t0[16], t1[16];
        cookie_tag16(byte_view_mut_n<16>{ t0 }, key32, ep, nonce, now_ms);
        cookie_tag16(byte_view_mut_n<16>{ t1 }, key32, ep, nonce, now_ms - 1024);
        return cl::poly1305::ct_equal(recv16, as_view(t0)) ||
               cl::poly1305::ct_equal(recv16, as_view(t1));
    }

    enum class Protocol : u8 { TCP, UDP };
    enum class Error : u8 {
        None = 0,
        WouldBlock,
        Again,
        Closed,
        Generic,
        ConnReset,
    };

    static char_view error_str(Error r) noexcept {
        switch (r) {
            case Error::None:       return "None";
            case Error::WouldBlock: return "WouldBlock";
            case Error::Again:      return "Again";
            case Error::Closed:     return "Closed";
            case Error::Generic:    return "Generic";
            case Error::ConnReset:  return "ConnReset";
            default: return "?";
        }
    }

    struct Socket {
#if defined(_WIN32)
        using native_t = SOCKET;
        static IO_CONSTEXPR_VAR native_t invalid_native = INVALID_SOCKET;
#else
        using native_t = int;
        static IO_CONSTEXPR_VAR native_t invalid_native = -1;
#endif

    private:
        native_t _s{ invalid_native };
        Error    _error{ Error::Closed };
        Protocol _proto{ Protocol::UDP };
        bool     _opened{ false };

    public:
        Socket() noexcept { startup(); }
        ~Socket() noexcept { close(); }

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        Socket(Socket&& o) noexcept : _s(o._s), _error(o._error), _proto(o._proto), _opened(o._opened) {
            o._s=invalid_native; o._opened=false; o._error=Error::Closed;
        }
        Socket& operator=(Socket&& o) noexcept {
            if (this == &o) return *this;
            close();
            _s=o._s; _error=o._error; _proto=o._proto; _opened=o._opened;
            o._s=invalid_native; o._opened=false; o._error=Error::Closed;
            return *this;
        }

        IO_NODISCARD bool is_open() const noexcept { return _opened; }
        IO_NODISCARD Protocol protocol() const noexcept { return _proto; }
        IO_NODISCARD native_t native() const noexcept { return _s; }
        IO_NODISCARD Error error() const noexcept { return _error; }
        IO_NODISCARD char_view error_str() const noexcept { return ::io::error_str(_error); }

        IO_NODISCARD bool open(Protocol proto) noexcept {
            _proto = proto;
#if defined(_WIN32)
            const int type = (proto == Protocol::TCP) ? SOCK_STREAM : SOCK_DGRAM;
            _s = ::socket(AF_INET, type, (proto == Protocol::TCP) ? IPPROTO_TCP : IPPROTO_UDP);
            if (_s == invalid_native) { update_last_error(); return false; }
#elif defined(__linux__)
            using namespace native;
            const int type = (proto == Protocol::TCP) ? k_sock_stream : k_sock_dgram;
            const int pr   = (proto == Protocol::TCP) ? k_ipproto_tcp  : k_ipproto_udp;
            const long r = sys_socket(k_af_inet, type, pr);
            if (is_err(r)) { update_last_error_from_ret(r); return false; }
            _s = (int)r;
#else
#   error "close(): Not implemented"
#endif
            _opened = true;
            _error = Error::None;
            return true;
        }
        void close() noexcept {
            if (_s == invalid_native) return;
#if defined(_WIN32)
            ::closesocket(_s);
#elif defined(__linux__)
            (void)native::sys_close(_s);
#else
#   error "close(): Not implemented"
#endif
            _s = invalid_native;
            _opened = false;
            _error = Error::Closed;
        }
        IO_NODISCARD bool set_blocking(bool blocking) noexcept {
            if (!_opened) { _error = Error::Generic; return false; }
#if defined(_WIN32)
            unsigned long mode = blocking ? 0 : 1;
            if (::ioctlsocket(_s, FIONBIO, &mode) != 0) { update_last_error(); return false; }
            return true;
#elif defined(__linux__)
            using namespace native;
            const long fl = sys_fcntl(_s, k_f_getfl, 0);
            if (is_err(fl)) { update_last_error_from_ret(fl); return false; }
            const long newfl = blocking ? (fl & ~((long)k_o_nonblock))
                                        : (fl |  ((long)k_o_nonblock));
            const long r = sys_fcntl(_s, k_f_setfl, newfl);
            if (is_err(r)) { update_last_error_from_ret(r); return false; }
            return true;
#else
#   error "set_blocking(): Not implemented"
#endif
        }
        IO_NODISCARD bool bind(Endpoint ep) noexcept {
            if (!_opened) { _error = Error::Generic; return false; }
#if defined(_WIN32)
            sockaddr_in a{};
            a.sin_family      = AF_INET;
            a.sin_addr.s_addr = ep.addr_be;
            a.sin_port        = ep.port_be;
            const int r = ::bind(_s, reinterpret_cast<sockaddr*>(&a), (int)sizeof(a));
            if (r == 0) { _error = Error::None; return true; }
            update_last_error();
            return false;
#elif defined(__linux__)
            using namespace native;
            sockaddr_in a{};
            a.sin_family      = (u16)k_af_inet;
            a.sin_addr.s_addr = ep.addr_be;
            a.sin_port        = ep.port_be;
            const long r = sys_bind(_s, &a, (unsigned)sizeof(a));
            if (!is_err(r)) { _error = Error::None; return true; }
            update_last_error_from_ret(r);
            return false;
#else
#   error "bind(): Not implemented"
#endif
        }
        // UDP
        IO_NODISCARD int send_to(Endpoint to, byte_view v) noexcept {
            if (!_opened || _proto != Protocol::UDP) { _error = Error::Generic; return -1; }
            if (!v.data() && v.size() != 0) { _error = Error::Generic; return -1; }
#if defined(_WIN32)
            sockaddr_in a{};
            a.sin_family      = AF_INET;
            a.sin_addr.s_addr = to.addr_be;
            a.sin_port        = to.port_be;
            const int r = ::sendto(_s, reinterpret_cast<const char*>(v.data()), (int)v.size(), 0,
                reinterpret_cast<sockaddr*>(&a), (int)sizeof(a));
            if (r != SOCKET_ERROR) { _error = Error::None; return r; }
            update_last_error();
            return -1;
#elif defined(__linux__)
            using namespace native;
            sockaddr_in a{};
            a.sin_family = (u16)k_af_inet;
            a.sin_addr.s_addr = to.addr_be;
            a.sin_port = to.port_be;
            const long r = sys_sendto(_s, v.data(), (unsigned long)v.size(), 0, &a, (socklen_t)sizeof(a));
            if (!is_err(r)) { _error = Error::None; return (int)r; }
            update_last_error_from_ret(r);
            return -1;
#else
#   error "send_to(): Not implemented"
#endif
        }
        IO_NODISCARD int recv_from(Endpoint& out_from, byte_view_mut v) noexcept {
            if (!_opened || _proto != Protocol::UDP) { _error = Error::Generic; return -1; }
            if (!v.data() && v.size() != 0) { _error = Error::Generic; return -1; }
#if defined(_WIN32)
            sockaddr_in a{};
            int alen = (int)sizeof(a);
            const int r = ::recvfrom(_s, reinterpret_cast<char*>(v.data()), (int)v.size(), 0,
                reinterpret_cast<sockaddr*>(&a), &alen);
            if (r >= 0) {
                out_from.addr_be = a.sin_addr.s_addr;
                out_from.port_be = a.sin_port;
                _error = Error::None;
                return r; // may be 0 for a valid empty UDP datagram
            }
            update_last_error();
            return -1;
#elif defined(__linux__)
            using namespace native;
            sockaddr_in a{};
            socklen_t alen = (socklen_t)sizeof(a);
            const long r = sys_recvfrom(_s, v.data(), (unsigned long)v.size(), 0, &a, &alen);
            if (!is_err(r)) {
                out_from.addr_be = a.sin_addr.s_addr;
                out_from.port_be = a.sin_port;
                _error = Error::None;
                return (int)r;
            }
            update_last_error_from_ret(r);
            return -1;
#else
#   error "recv_from(): Not implemented"
#endif
        }

    private:
        
#if defined(_WIN32)
        void update_last_error() noexcept {
            const int e = WSAGetLastError();
            if (e == WSAEWOULDBLOCK) { _error = Error::WouldBlock; return; }
            if (e == WSAEINTR)       { _error = Error::Again;      return; }
            if (e == WSAECONNRESET)  { _error = Error::ConnReset;     return; }
            _error = Error::Generic;
#elif defined(__linux__)
        void update_last_error_from_ret(long r) noexcept {
            using namespace native;
            if (!is_err(r)) { _error = Error::None; return; }
            const long e = err_no(r);
            if (e == k_eagain || e == k_ewouldblock) { _error = Error::WouldBlock; return; }
            if (e == k_eintr)                        { _error = Error::Again; return; }
            if (e == k_econnreset)                   { _error = Error::ConnReset; return; }
            _error = Error::Generic;
#else
#   error "update_last_error(): Not implemented"
#endif
        }

        bool startup() noexcept {
#if defined(_WIN32)
            static bool ready = false;
            if (!ready) {
                WSADATA w{};
                if (::WSAStartup(MAKEWORD(2, 2), &w) != 0) { _error = Error::Generic; return false; }
                ready = true;
            }
#endif
            _error = Error::None;
            return true;
        }
    };

    // -------------- replay/window (128) --------------------------------
    struct SeqWindow128 {
        u32 last = 0;   // highest seq seen
        u64 bits0 = 0;  // bit0 => last, bit1 => last-1, ... bit63 => last-63
        u64 bits1 = 0;  // bit0 => last-64, ... bit63 => last-127

        // Returns true if `seq` is NEW (accepted), false if duplicate/too old.
        IO_NODISCARD bool accept(u32 seq) noexcept {
            if (seq == 0) return false;

            if (last == 0) {
                last = seq;
                bits0 = 1ull; bits1 = 0;
                return true;
            }

            if (seq > last) {
                const u32 delta = seq - last;
                if (delta >= 128) {
                    bits0 = 1ull; bits1 = 0;
                }
                else {
                    // shift window left by delta
                    for (u32 i = 0; i < delta; ++i) {
                        bits1 = (bits1 << 1) | (bits0 >> 63);
                        bits0 = (bits0 << 1);
                    }
                    bits0 |= 1ull;
                }
                last = seq;
                return true;
            }

            const u32 behind = last - seq;
            if (behind < 64) {
                const u64 mask = 1ull << behind;
                if (bits0 & mask) return false;
                bits0 |= mask;
                return true;
            }
            if (behind < 128) {
                const u32 b = behind - 64;
                const u64 mask = 1ull << b;
                if (bits1 & mask) return false;
                bits1 |= mask;
                return true;
            }
            return false;
        }

        // Returns ack (latest received) and ack_bits for [ack-1 .. ack-64].
        // ack_bits bit0 => (ack-1) received, bit63 => (ack-64) received.
        IO_NODISCARD IO_CONSTEXPR u32 ack() const noexcept { return last; }

        IO_NODISCARD IO_CONSTEXPR u64 ack_bits64() const noexcept {
            // Need [ack-1 .. ack-64]
            const u64 top = (bits1 & 1ull) ? 0x8000000000000000ull : 0ull; // no shift helper
            return (bits0 >> 1) | top;
        }
    };

    struct AckState {
        u64 ack_bits = 0;
        u32 ack = 0;
    };
    IO_NODISCARD static inline AckState make_ack_state(const SeqWindow128& w) noexcept {
        AckState a{};
        a.ack = w.last;
        // bits0>>1 gives [ack-1 .. ack-63] in bits [0..62]
        // ack-64 lays in bits1 bit0 -> put in ack_bits bit63
        a.ack_bits = (w.bits0 >> 1) | ((w.bits1 & 1ull) << 63);
        return a;
    }
    IO_NODISCARD static inline bool is_acked_by(u32 seq, u32 ack, u64 ack_bits) noexcept {
        if (seq == 0 || ack == 0) return false;
        if (seq == ack) return true;
        if (seq > ack) return false;

        const u32 d = ack - seq; // seq behind ack
        if (d >= 1 && d <= 64) {
            const u32 bit = d - 1; // d=1 -> bit0 (ack-1)
            return ((ack_bits >> bit) & 1ull) != 0;
        }
        return false;
    }

    // ====================================================================
    // UDP header
    // ====================================================================

    enum class UdpChan : u8 { Unreliable = 0, Reliable = 1 };

#pragma pack(push, 1)
    struct UdpHeader {
        u64 ack_bits;
        u32 magic;
        u32 seq;
        u32 ack;
        u16 version;
        u16 payload_len;
        u8  chan;        // UdpChan
        u8  type;        // user message type
    };
#pragma pack(pop)
    static_assert(sizeof(UdpHeader) == 26, "UdpHeader size");

    // Convert host->wire in-place (network order).
    static inline void udp_header_host_to_wire(UdpHeader& h) noexcept {
        h.magic       = ::io::h2nl(h.magic);
        h.version     = ::io::h2ns(h.version);
        h.seq         = ::io::h2nl(h.seq);
        h.ack         = ::io::h2nl(h.ack);
        h.ack_bits    = ::io::h2nll(h.ack_bits);
        h.payload_len = ::io::h2ns(h.payload_len);
    }

    // Convert wire->host in-place.
    static inline void udp_header_wire_to_host(UdpHeader& h) noexcept {
        h.magic       = ::io::n2hl(h.magic);
        h.version     = ::io::n2hs(h.version);
        h.seq         = ::io::n2hl(h.seq);
        h.ack         = ::io::n2hl(h.ack);
        h.ack_bits    = ::io::n2hll(h.ack_bits);
        h.payload_len = ::io::n2hs(h.payload_len);
    }

    enum class DisconnectReason : u8 {
        Unknown          = 0,
        HandshakeTimeout = 1,
        PeerTimeout      = 2,
        LocalReset       = 3,
        ServerClosed     = 4,
    };

    static const char* disconnect_reason_str(DisconnectReason dr) noexcept {
        switch(dr) {
            case DisconnectReason::Unknown: return "Unknown";
            case DisconnectReason::HandshakeTimeout: return "HandshakeTimeout";
            case DisconnectReason::PeerTimeout: return "PeerTimeout";
            case DisconnectReason::LocalReset: return "LocalReset";
            case DisconnectReason::ServerClosed: return "ServerClosed";
            default: return "?";
        }
    }

    // -------------- callbacks ------------------------------------------
    struct UdpCallbacks {
        void (*on_packet)(void* ud, Endpoint from, u8 type, UdpChan chan, byte_view payload) = nullptr;
        void (*on_drop)(void* ud, Endpoint from, Error why, DropReason reason) = nullptr;
        void (*on_established)(void* ud, Endpoint peer, u32 session_id) = nullptr;
        void (*on_disconnect)(void* ud, Endpoint peer, u32 session_id, DisconnectReason why) = nullptr;
        void* ud = nullptr;
    };

    // ============================================================================
    // Packet arena + ref queue (EventLoop-owned).
    // Stores FULL datagram bytes: [UdpHeader | payload] contiguous.
    // ============================================================================

    struct packet_ref {
        u32 off;     // offset in arena
        u16 len;     // total bytes (header + payload) fits u16 for MTU-ish sizes
        Endpoint to;
    };

    struct packet_arena {
        unique_bytes mem{};
        u32 cap = 0;

        // ring pointers
        u32 head = 0; // write
        u32 tail = 0; // read (oldest not-yet-freed)
        u32 used = 0;

        IO_NODISCARD bool init(u32 capacity_bytes, usize alignment = 16) noexcept {
            mem.reset(nullptr);
            cap = head = tail = used = 0;

            u8* p = static_cast<u8*>(alloc_aligned(capacity_bytes, alignment));
            if (!p) return false;
            mem = unique_bytes{ p };
            cap = capacity_bytes;
            return true;
        }

        // allocate contiguous block (no split). returns offset in ring buffer.
        IO_NODISCARD bool alloc(u32 bytes, u32& out_off) noexcept {
            if (!mem.get() || bytes == 0) return false;
            if (bytes > cap) return false;
            if (bytes > (cap - used)) return false; // not enough free space

            // if head >= tail, we may have space [head..cap) and [0..tail)
            if (head >= tail) {
                const u32 right = cap - head;
                if (bytes <= right) {
                    out_off = head;
                    head += bytes;
                    if (head == cap) head = 0;
                    used += bytes;
                    return true;
                }
                // try wrap to 0, require bytes <= tail
                if (bytes <= tail) {
                    out_off = 0;
                    head = bytes;
                    used += bytes;
                    return true;
                }
                return false;
            }

            // head < tail: free block is [head..tail)
            const u32 free_mid = tail - head;
            if (bytes <= free_mid) {
                out_off = head;
                head += bytes;
                used += bytes;
                return true;
            }
            return false;
        }

        // consume bytes from tail (MUST match FIFO send order)
        void free_front(u32 bytes) noexcept {
            if (bytes == 0 || used == 0) return;
            if (bytes > used) bytes = used;

            tail += bytes;
            if (tail >= cap) tail -= cap;
            used -= bytes;
        }

        IO_NODISCARD u8* ptr(u32 off) noexcept { return mem.get() ? (mem.get() + off) : nullptr; }
        IO_NODISCARD const u8* ptr(u32 off) const noexcept { return mem.get() ? (mem.get() + off) : nullptr; }
    };

    template<u16 MaxPayload, usize MaxInflight>
    struct reliable_outbox {
        static IO_CONSTEXPR_VAR u32 SLOT_STRIDE = (u32)sizeof(UdpHeader) + (u32)MaxPayload;

        struct reliable_packet {
            u64 last_send_ms = 0;
            u32 slot = 0;         // slot index [0..MaxInflight)
            u32 rto_ms = 200;     // start fixed; you can add RTT later
            u32 seq = 0;
            u16 len = 0;          // total datagram bytes (header+payload)
            u8  retries = 0;
            bool used = false;
        };

        reliable_packet* pkts = nullptr; // [MaxInflight]
        u8* slots = nullptr;             // [MaxInflight * SLOT_STRIDE]

        IO_NODISCARD bool bind(void* pkts_mem, void* slots_mem) noexcept {
            pkts = (reliable_packet*)pkts_mem;
            slots = (u8*)slots_mem;
            if (!pkts || !slots) return false;

            for (usize i = 0; i < MaxInflight; ++i) pkts[i] = reliable_packet{};
            return true;
        }

        inline u8* slot_ptr(u32 slot_i) noexcept { return slots + slot_i * SLOT_STRIDE; }
        inline const u8* slot_ptr(u32 slot_i) const noexcept { return slots + slot_i * SLOT_STRIDE; }

        IO_NODISCARD bool reserve(reliable_packet*& out_rp, u32& out_slot) noexcept {
            for (u32 i = 0; i < (u32)MaxInflight; ++i) {
                if (!pkts[i].used) {
                    pkts[i] = reliable_packet{};
                    pkts[i].used = true;
                    pkts[i].slot = i;      // 1:1 slot mapping
                    out_rp = &pkts[i];
                    out_slot = i;
                    return true;
                }
            }
            return false;
        }

        inline void free_by_index(u32 i) noexcept {
            if (i < (u32)MaxInflight) pkts[i].used = false;
        }

        inline void reset_used_only() noexcept {
            for (u32 i = 0; i < (u32)MaxInflight; ++i) pkts[i].used = false;
        }
    };

    struct linear_arena {
        u8* base = nullptr;
        u32 cap = 0;
        u32 at = 0;

        IO_NODISCARD bool init(u32 bytes, usize alignment = 16) noexcept {
            base = (u8*)alloc_aligned(bytes, alignment);
            if (!base) { cap=at=0; return false; }
            cap = bytes;
            at = 0;
            return true;
        }
        ~linear_arena() noexcept { if (base) free_aligned(base); }

        static IO_CONSTEXPR_VAR u32 align_up_u32(u32 x, u32 a) noexcept { return (x + (a-1u)) & ~(a-1u); }
        template<class T> static IO_CONSTEXPR_VAR u32 alignof_u32() noexcept { return (u32)alignof(T); }

        IO_NODISCARD void* alloc(u32 bytes, u32 align) noexcept {
            u32 p = align_up_u32(at, (u32)align);
            if (bytes > (cap-p)) return nullptr;
            void* out = base + p;
            at = p + bytes;
            return out;
        }
    };

    template<usize MaxPackets>
    struct send_queue {
        packet_ref* q = nullptr; // heap/arena-backed
        u32 r = 0, w = 0, n = 0;

        IO_NODISCARD bool init(unique_bytes& backing, u32 count) noexcept {
            // count == MaxPackets
            q = (packet_ref*)backing.get();
            if (!q) return false;
            for (u32 i = 0; i < count; ++i) q[i] = packet_ref{};
            r = w = n = 0;
            return true;
        }
        IO_NODISCARD bool empty() const noexcept { return n == 0; }
        IO_NODISCARD bool full(u32 cap) const noexcept { return n == cap; }
        IO_NODISCARD bool push(const packet_ref& pr, u32 cap) noexcept {
            if (full(cap)) return false;
            q[w] = pr;
            w = (w + 1u) % cap;
            ++n;
            return true;
        }
        IO_NODISCARD bool front(packet_ref& out) const noexcept {
            if (empty()) return false;
            out = q[r];
            return true;
        }
        void drop_front(u32 cap) noexcept {
            if (empty()) return;
            r = (r + 1u) % cap;
            --n;
        }
    };

    // ====================================================================
    // EventLoop backend (WSAPoll / epoll) + enqueue_send()
    // ====================================================================

    template<u16 MaxPayload = 1200u, usize MaxPackets = 2048u>
    struct EventLoop {
        static_assert(MaxPayload > 0, "MaxPayload must be > 0");
        static_assert(MaxPackets > 0, "MaxPackets must be > 0");

        static IO_CONSTEXPR_VAR u32 SENDQ_CAP = (u32)MaxPackets;

        using rel_box = reliable_outbox<MaxPayload, REL_INFLIGHT>;
        using rel_packet = typename reliable_outbox<MaxPayload, REL_INFLIGHT>::reliable_packet;

        // Peer state for ack/ack_bits integration.
        struct udp_peer_state {
            SeqWindow128 recv_win{}; // tracks received seqs (for ack generation)
            Endpoint ep{};
            reliable_outbox<MaxPayload, REL_INFLIGHT> rel{};

            // Last ack info we received FROM remote (for future reliable/resend).
            u64 remote_ack_bits = 0;
            u32 remote_ack = 0;
            u32 send_seq = 0;        // next outgoing seq

            // ---- session / handshake ----
            enum : u8 { HS_NONE = 0, HS_COOKIE_SENT = 1, HS_ESTABLISHED = 2 } hs = HS_NONE;
            u32 session_id = 0;
            u32 client_nonce = 0;     // server stores what client presented
            u16 mtu = DEFAULT_MTU;    // negotiated
            u16 features = 0;

            u64 last_rx_ms = 0;
            u64 last_tx_ms = 0;
            u64 hs_deadline_ms = 0;

            bool used = false;
            bool rel_ready = false;
        };


        linear_arena perm{};
        packet_arena tx_arena{};

        send_queue<MaxPackets> sendq{}; // [SENDQ_CAP]
        unique_bytes sendq_mem{};       // [SENDQ_CAP]

        unique_bytes peers_mem{}; // [MAX_PEERS]
        udp_peer_state* peers{};  // [MAX_PEERS]

        UdpCallbacks* cb_live = nullptr;

        // @TODO split in union fields below to separate server/client

        // HS server
        u8 server_cookie_secret[32]{}; // set by server once on init
        u32 server_next_session_id = 1;

        // HS client
        Endpoint server_ep{};
        u32 client_nonce = 0x12345678u; // set by user on HS, could be wiped when established
        u32 session_id = 0;
        u16 desired_mtu = DEFAULT_MTU;
        u16 desired_features = FEATURE_COOKIE;
        
        bool is_server = true; // or set from outside
        bool running{};
        void stop() noexcept { running = false; }

        // MUST be called before run_udp()
        IO_NODISCARD bool init(u32 perm_bytes, u32 tx_bytes, bool is_server_) noexcept {
#ifdef __linux__
            ep = -1;
#endif
            is_server = is_server_;
            server_next_session_id = 1;
            running = true;
            if (is_server) {
                (void)os_entropy(&server_cookie_secret, sizeof(server_cookie_secret));
                if (server_cookie_secret == 0) { // fallback non-zero
                    for (int i=0; i<32; ++i) server_cookie_secret[i] = i*2+1;
                }
            }

            if (!perm.init(perm_bytes)) return false;
            if (!tx_arena.init(tx_bytes)) return false;
            
            sendq_mem.reset((u8*)alloc_aligned((u32)(MaxPackets * sizeof(packet_ref)), 16));
            if (!sendq_mem.get()) return false;
            if (!sendq.init(sendq_mem, (u32)MaxPackets)) return false;

            // check arena size in debug; in release we pass UNRELIABLE connections if not checked before
#ifndef NDEBUG
            IO_CONSTEXPR_VAR u32 need_perm = calc_perm_bytes_for_rel();
            assert(perm_bytes >= need_perm);
#endif

            peers_mem.reset((u8*)alloc_aligned((u32)(MAX_PEERS * sizeof(udp_peer_state)), alignof(udp_peer_state)));
            if (!peers_mem.get()) return false;

            peers = (udp_peer_state*)peers_mem.get();
            for (u32 i = 0; i < (u32)MAX_PEERS; ++i)
                peers[i] = udp_peer_state{}; // zero-init all peers (no memset)

            // bind reliable buffers per peer
            for (usize i = 0; i < MAX_PEERS; ++i) {
                udp_peer_state& ps = peers[i];

                void* pkts_mem = perm.alloc((u32)(REL_INFLIGHT * sizeof(rel_packet)), alignof(rel_packet));
                void* slots_mem = perm.alloc((u32)(REL_INFLIGHT * rel_box::SLOT_STRIDE), 16);
                assert(pkts_mem && slots_mem);
                // if false -> reliable is disabled for current peer
                ps.rel_ready = ps.rel.bind(pkts_mem, slots_mem);
                assert(ps.rel_ready);
            }

            return true;
        }
        IO_NODISCARD inline bool init(bool is_server_) noexcept {
            IO_CONSTEXPR_VAR u32 perm_need = calc_perm_bytes_for_rel();
            // TX: rough, but safety guarranted. Ring keeps full datagrams
            // header + MaxPayload on packet.
            IO_CONSTEXPR_VAR u32 max_datagram = (u32)sizeof(UdpHeader) + (u32)MaxPayload;
            // MaxPackets * max_datagram + slack for wrap/align.
            IO_CONSTEXPR_VAR u32 tx_need = (u32)(MaxPackets * (usize)max_datagram
                                          + (usize)max_datagram + 16u);
            return init(perm_need, tx_need, is_server_);
        }

        inline ~EventLoop() noexcept {
            if (!cb_live || !peers) return;

            for (usize i = 0; i < MAX_PEERS; ++i) {
                udp_peer_state& ps = peers[i];
                if (!ps.used) continue;
                // Explicitly notify all peers that we're closed
                handle_disconnect(*cb_live, ps, DisconnectReason::ServerClosed);
            }
#ifdef __linux__
            if (ep >= 0) { (void)native::sys_close(ep); ep = -1; }
#endif
        }

    private:
        void flush_outgoing(Socket& udp) noexcept {
            // --- flush outgoing (zero-copy send from arena) ---
            for (;;) {
                packet_ref pr{};
                if (!sendq.front(pr)) break;
                const u8* p = tx_arena.ptr(pr.off);
                if (!p) { // corrupted ref -> drop
                    sendq.drop_front(SENDQ_CAP);
                    tx_arena.free_front(pr.len);
                    continue;
                }
                const int sent = udp.send_to(pr.to, { p, (usize)pr.len });
                if (sent == (int)pr.len) {
                    // success: remove from queue and free arena bytes
                    sendq.drop_front(SENDQ_CAP);
                    tx_arena.free_front(pr.len);
                    continue;
                }
                // send failed
                if (udp.error() == Error::WouldBlock || udp.error() == Error::Again) {
                    // keep packet queued; DO NOT free arena; try later
                    break;
                }
                // hard error: drop packet (v1 behavior)
                sendq.drop_front(SENDQ_CAP);
                tx_arena.free_front(pr.len);
            }
        }

        static inline void on_remote_ack(udp_peer_state& ps) noexcept {
            if (!ps.rel_ready) return;
            const u32 ack = ps.remote_ack;
            const u64 ack_bits = ps.remote_ack_bits;

            for (u32 i = 0; i < (u32)REL_INFLIGHT; ++i) {
                auto& rp = ps.rel.pkts[i];
                if (!rp.used) continue;
                if (is_acked_by(rp.seq, ack, ack_bits)) {
                    ps.rel.free_by_index(i);
                }
            }
        }
        static inline bool is_handshake_msg(u8 type) noexcept {
            return type == MSG_HELLO || type == MSG_COOKIE || type == MSG_HELLO2 || type == MSG_WELCOME;
        }
        static inline u16 mtu_payload_cap(u16 mtu) noexcept {
            // mtu is datagram bytes; cap payload so header fits.
            // mtu must be >= sizeof(UdpHeader)
            const u32 h = (u32)sizeof(UdpHeader);
            if ((u32)mtu <= h) return 0;
            const u32 cap = (u32)mtu - h;
            return (cap > 0xFFFFu) ? 0xFFFFu : (u16)cap;
        }
        IO_NODISCARD static inline u16 effective_payload_cap(const udp_peer_state& ps) noexcept {
            if (ps.hs == udp_peer_state::HS_ESTABLISHED) {
                const u16 cap = mtu_payload_cap(ps.mtu);
                return cap;
            }
            return (u16)MaxPayload; // pre-established: allow only compile-time cap (for handshake)
        }
        
        static IO_CONSTEXPR_VAR u32 calc_perm_bytes_for_rel() noexcept {
            u32 at = 0;
            for (u32 i = 0; i < (u32)MAX_PEERS; ++i) {
                at = linear_arena::align_up_u32(at, linear_arena::alignof_u32<rel_packet>());
                at += (u32)(REL_INFLIGHT * sizeof(rel_packet));

                at = linear_arena::align_up_u32(at, 16u);
                at += (u32)(REL_INFLIGHT * rel_box::SLOT_STRIDE);
            }
            return at;
        }

     public:
        static inline void peer_reset_keep_rel(udp_peer_state& ps) noexcept {
            // preserve reliable bindings
            rel_box rel = ps.rel;
            const bool rel_ready = ps.rel_ready;

            ps = udp_peer_state{};
            ps.rel = rel;
            ps.rel_ready = rel_ready;

            if (ps.rel_ready) ps.rel.reset_used_only();
        }

        IO_NODISCARD udp_peer_state* find_peer(Endpoint ep) noexcept {
            if (!peers) return nullptr;
            for (usize i = 0; i < MAX_PEERS; ++i) {
                if (peers[i].used && endpoint_eq(peers[i].ep, ep))
                    return &peers[i];
            }
            return nullptr;
        }
        IO_NODISCARD udp_peer_state* get_peer_create(Endpoint ep) noexcept {
            // 1) find existing
            if (udp_peer_state* ps = find_peer(ep)) return ps;

            // 2) allocate new slot
            for (usize i = 0; i < MAX_PEERS; ++i) {
                udp_peer_state& ps = peers[i];
                if (!ps.used) {
                    peer_reset_keep_rel(ps);

                    ps.used = true;
                    ps.ep = ep;
                    ps.send_seq = 1;
                    ps.recv_win = SeqWindow128{};
                    ps.remote_ack = 0;
                    ps.remote_ack_bits = 0;
                    return &ps;
                }
            }
            return nullptr; // table full
        }

        // Build datagram in arena: [UdpHeader | payload], queue stores only (off/len/to)
        IO_NODISCARD bool enqueue_send(Endpoint to,
            u8 type,
            UdpChan chan,
            u32 seq, u32 ack, u64 ack_bits,
            byte_view payload) noexcept
        {
            if (!payload.data() && payload.size() != 0) return false; // allow empty
            if (payload.size() > MaxPayload) return false;

            const u32 total = (u32)sizeof(UdpHeader) + (u32)payload.size();
            if (total > 0xFFFFu) return false;
            if (sendq.full(SENDQ_CAP)) return false;

            u32 off = 0;
            if (!tx_arena.alloc(total, off)) return false;

            u8* dst = tx_arena.ptr(off);
            if (!dst) return false;

            // Header
            UdpHeader h{};
            h.magic = UDP_MAGIC;
            h.version = UDP_VERSION;
            h.chan = (u8)chan;
            h.type = type;
            h.seq = seq;
            h.ack = ack;
            h.ack_bits = ack_bits;
            h.payload_len = (u16)payload.size();

            udp_header_host_to_wire(h);

            // Copy header bytes
            const u8* hs = (const u8*)&h;
            for (u32 i = 0; i < (u32)sizeof(UdpHeader); ++i) dst[i] = hs[i];

            // Copy payload bytes (ONE copy total)
            u8* pd = dst + sizeof(UdpHeader);
            for (u32 i = 0; i < (u32)payload.size(); ++i) pd[i] = payload.data()[i];

            packet_ref pr{};
            pr.off = off;
            pr.len = (u16)total; // total datagram bytes
            pr.to = to;
            return sendq.push(pr, SENDQ_CAP);
        }
        // --- packet_ref: add a flag so we can later extend (optional) ---
        // For now we keep arena-only refs, but we add enqueue_send_raw() helper.
        IO_NODISCARD bool enqueue_send_raw(Endpoint to, const u8* bytes, u32 len) noexcept {
            if (!bytes || len == 0) return false;
            if (len > 0xFFFFu) return false;
            if (sendq.full(SENDQ_CAP)) return false;

            u32 off = 0;
            if (!tx_arena.alloc(len, off)) return false;

            u8* dst = tx_arena.ptr(off);
            if (!dst) return false;

            for (u32 i = 0; i < len; ++i) dst[i] = bytes[i];

            packet_ref pr{};
            pr.off = off;
            pr.len = (u16)len;
            pr.to = to;
            return sendq.push(pr, SENDQ_CAP);
        }

        IO_NODISCARD bool send_unreliable(udp_peer_state& ps, u8 type, byte_view payload, u64 now_ms) noexcept {
            // Allow only handshake before established (except keepalive handling elsewhere).
            if (ps.hs != udp_peer_state::HS_ESTABLISHED && !is_handshake_msg(type))
                return false;

            const u16 cap = effective_payload_cap(ps);
            if (cap == 0) return false;
            if (payload.size() > cap) return false;

            const u32 seq = ps.send_seq++;
            const u32 ack = ps.recv_win.ack();
            const u64 ack_bits = ps.recv_win.ack_bits64();

            ps.last_tx_ms = now_ms;
            return enqueue_send(ps.ep, type, UdpChan::Unreliable, seq, ack, ack_bits, payload);
        }
        IO_NODISCARD bool send_reliable(udp_peer_state& ps, u8 type, byte_view payload, u64 now_ms) noexcept {
            if (!ps.rel_ready) return false;

            // Reliable only after established, but handshake msgs are allowed.
            if (ps.hs != udp_peer_state::HS_ESTABLISHED && !is_handshake_msg(type))
                return false;

            const u16 cap = effective_payload_cap(ps);
            if (cap == 0) return false;
            if (payload.size() > cap) return false;

            const u32 total = (u32)sizeof(UdpHeader) + (u32)payload.size();
            if (total > 0xFFFFu) return false;
            if (payload.size() > MaxPayload) return false; // hard compile-time safety

            typename reliable_outbox<MaxPayload, REL_INFLIGHT>::reliable_packet* rp = nullptr;
            u32 slot = 0;
            if (!ps.rel.reserve(rp, slot)) return false;

            u8* dst = ps.rel.slot_ptr(slot);
            if (!dst) { ps.rel.free_by_index(slot); return false; }

            const u32 seq = ps.send_seq++;
            const u32 ack = ps.recv_win.ack();
            const u64 ack_bits = ps.recv_win.ack_bits64();

            UdpHeader h{};
            h.magic = UDP_MAGIC;
            h.version = UDP_VERSION;
            h.chan = (u8)UdpChan::Reliable;
            h.type = type;
            h.seq = seq;
            h.ack = ack;
            h.ack_bits = ack_bits;
            h.payload_len = (u16)payload.size();
            udp_header_host_to_wire(h);

            // Copy header
            for (u32 i = 0; i < (u32)sizeof(UdpHeader); ++i)
                dst[i] = ((u8*)&h)[i];

            // Copy payload
            for (u32 i = 0; i < (u32)payload.size(); ++i)
                dst[sizeof(UdpHeader) + i] = payload.data()[i];

            rp->seq = seq;
            rp->len = (u16)total;
            rp->slot = slot;
            rp->last_send_ms = now_ms;
            rp->rto_ms = 200;
            rp->retries = 0;

            ps.last_tx_ms = now_ms;

            // send bytes directly from outbox slot
            if (!enqueue_send_raw(ps.ep, dst, total)) {
                ps.rel.free_by_index(slot);
                return false;
            }
            return true; 
        }
        void send_keepalive_unreliable(udp_peer_state& ps) noexcept {
            msg_keepalive2 k{};
            k.session_id = ::io::h2nl(ps.session_id);

            const u32 seq = ps.send_seq++;
            const u32 ack = ps.recv_win.ack();
            const u64 ack_bits = ps.recv_win.ack_bits64();

            (void)enqueue_send(ps.ep, MSG_KEEPALIVE, UdpChan::Unreliable,
                seq, ack, ack_bits,
                byte_view((u8*)&k, sizeof(k)));
        }

        static inline void handle_disconnect(UdpCallbacks& cb, udp_peer_state& ps, DisconnectReason why) noexcept {
            const bool was_used = ps.used;
            const bool was_established = (ps.hs == udp_peer_state::HS_ESTABLISHED);
            const u32 sid = ps.session_id;

            // Notify only if we had a real peer slot (and optionally only if established)
            if (was_used && cb.on_disconnect) {
                cb.on_disconnect(cb.ud, ps.ep, sid, why);
            }

            peer_reset_keep_rel(ps);
        }
        IO_NODISCARD bool disconnect_peer(Endpoint ep, DisconnectReason why, u64 now_ms) noexcept {
            udp_peer_state* ps = find_peer(ep);
            if (!ps) return false;
            // best-effort: send only if established
            if (ps->hs == udp_peer_state::HS_ESTABLISHED) {
                msg_disconnect d{};
                d.reason = (u8)why;
                d.session_id = ::io::h2nl(ps->session_id);

                const bool sent = ps->rel_ready // Prefer reliable if available, fallback to unreliable
                    ? send_reliable(*ps, MSG_DISCONNECT, byte_view{ (u8*)&d, sizeof(d) }, now_ms)
                    : send_unreliable(*ps, MSG_DISCONNECT, byte_view{ (u8*)&d, sizeof(d) }, now_ms);
                (void)sent;
            }
            if (cb_live) handle_disconnect(*cb_live, *ps, why);
            else peer_reset_keep_rel(*ps);

            return true;
        }


        void tick_reliable(u64 now_ms) noexcept {
            for (usize p = 0; p < MAX_PEERS; ++p) {
                udp_peer_state& ps = peers[p];
                if (!ps.used || !ps.rel_ready) continue;

                for (u32 i = 0; i < (u32)REL_INFLIGHT; ++i) {
                    auto& rp = ps.rel.pkts[i];
                    if (!rp.used) continue;

                    const u64 elapsed = (now_ms >= rp.last_send_ms) ? (now_ms - rp.last_send_ms) : 0;
                    if (elapsed < (u64)rp.rto_ms) continue;

                    // resend same datagram bytes (copy into send arena)
                    const u8* bytes = ps.rel.slot_ptr(rp.slot);
                    if (!bytes || rp.len == 0) { ps.rel.free_by_index(i); continue; }

                    (void)enqueue_send_raw(ps.ep, bytes, (u32)rp.len);

                    rp.last_send_ms = now_ms;
                    ++rp.retries;

                    // simple backoff (cap it)
                    u32 next_rto = rp.rto_ms * 2u;
                    if (next_rto < 100u) next_rto = 100u;
                    if (next_rto > 2000u) next_rto = 2000u;
                    rp.rto_ms = next_rto;

                    // optional: drop after N retries
                    if (rp.retries >= 10) {
                        ps.rel.free_by_index(i);
                    }
                }
            }
        }
        void tick_sessions(Socket& udp, UdpCallbacks& cb, u64 now_ms) noexcept {
            (void)udp;

            for (usize i = 0; i < MAX_PEERS; ++i) {
                udp_peer_state& ps = peers[i];
                if (!ps.used) continue;

                // handshake timeout
                if (ps.hs != udp_peer_state::HS_ESTABLISHED) {
                    if (ps.hs_deadline_ms != 0 && now_ms > ps.hs_deadline_ms) {
                        if (cb_live) handle_disconnect(*cb_live, ps, DisconnectReason::HandshakeTimeout);
                        else peer_reset_keep_rel(ps);
                        continue;
                    }
                }

                // established: keepalive + peer timeout
                if (ps.hs == udp_peer_state::HS_ESTABLISHED) {
                    if (ps.last_rx_ms != 0 && now_ms - ps.last_rx_ms > PEER_TIMEOUT_MS) {
                        if (cb_live) handle_disconnect(*cb_live, ps, DisconnectReason::PeerTimeout);
                        else peer_reset_keep_rel(ps);
                        continue;
                    }

                    if (ps.last_tx_ms == 0 || now_ms - ps.last_tx_ms >= KEEPALIVE_INTERVAL_MS) {
                        send_keepalive_unreliable(ps);
                        ps.last_tx_ms = now_ms;
                    }
                }
            }
        }
        
        IO_NODISCARD bool send_to_peer(Endpoint to, u8 type, UdpChan chan, byte_view payload, u64 now_ms) noexcept {
            udp_peer_state* ps = find_peer(to);
            if (!ps) return false;

            if (chan == UdpChan::Reliable) return send_reliable(*ps, type, payload, now_ms);
            return send_unreliable(*ps, type, payload, now_ms);
        }
        
        bool handle_handshake_server(udp_peer_state& ps, UdpCallbacks& cb, u8 type, byte_view payload, u64 now_ms) noexcept {
            if (type == MSG_HELLO) {
                if (payload.size() != sizeof(msg_hello)) return false;

                msg_hello h{};
                for (usize i = 0; i < sizeof(h); ++i) ((u8*)&h)[i] = payload.data()[i];

                // payload fields are in network order -> host
                h.mtu          = ::io::n2hs(h.mtu);
                h.features     = ::io::n2hs(h.features);
                h.client_nonce = ::io::n2hl(h.client_nonce);

                ps.client_nonce = h.client_nonce;
                ps.features = h.features;
                ps.mtu = clamp_mtu(h.mtu);

                // issue cookie challenge
                msg_cookie c{};
                /*c.cookie =*/ cookie_tag16(io::byte_view_mut_n<16>{ c.cookie },
                                            io::byte_view_n<32>{ server_cookie_secret },
                                            ps.ep, ps.client_nonce, now_ms);
                c.nonce_echo = ::io::h2nl(ps.client_nonce);

                // send unreliable (challenge); no session yet
                (void)send_to_peer(ps.ep, MSG_COOKIE, UdpChan::Unreliable,
                    byte_view{ (u8*)&c, sizeof(c) }, now_ms);

                ps.hs = udp_peer_state::HS_COOKIE_SENT;
                ps.hs_deadline_ms = now_ms + HANDSHAKE_TIMEOUT_MS;
                return true;
            }

            if (type == MSG_HELLO2) {
                if (payload.size() != sizeof(msg_hello2)) return false;
                const bool was_established = (ps.hs == udp_peer_state::HS_ESTABLISHED);
                msg_hello2 h2{};
                for (usize i = 0; i < sizeof(h2); ++i) ((u8*)&h2)[i] = payload.data()[i];

                // host
                const u32 nonce  = ::io::n2hl(h2.client_nonce);
                const u16 mtu    = ::io::n2hs(h2.mtu);
                const u16 feats  = ::io::n2hs(h2.features);

                if (!cookie_check(io::byte_view_n<16>{ h2.cookie },
                                  io::byte_view_n<32>{ server_cookie_secret },
                                  ps.ep, nonce, now_ms))
                    return false;
                ps.client_nonce = nonce;
                ps.features = feats;
                ps.mtu = clamp_mtu(mtu);

                if (ps.session_id == 0) ps.session_id = server_next_session_id++;
                ps.hs = udp_peer_state::HS_ESTABLISHED;

                if (!was_established && cb.on_established)
                    cb.on_established(cb.ud, ps.ep, ps.session_id);

                msg_welcome w{};
                w.mtu        = ::io::h2ns(ps.mtu);
                w.features   = ::io::h2ns(ps.features);
                w.session_id = ::io::h2nl(ps.session_id);

                (void)send_to_peer(ps.ep, MSG_WELCOME, UdpChan::Reliable,
                    byte_view{ (u8*)&w, sizeof(w) }, now_ms);
                return true;
            }

            return false;
        }
        bool handle_handshake_client(udp_peer_state& ps, UdpCallbacks& cb, u8 type, byte_view payload, u64 now_ms) noexcept {
            if (type == MSG_COOKIE) {
                if (payload.size() != sizeof(msg_cookie)) return false;
                msg_cookie c{};
                for (usize i = 0; i < sizeof(c); ++i) ((u8*)&c)[i] = payload.data()[i];

                const u32 nonce_echo = ::io::n2hl(c.nonce_echo);
                if (nonce_echo != client_nonce) return false;

                msg_hello2 h2{};
                for (int i = 0; i < 16; ++i)
                    h2.cookie[i] = c.cookie[i];
                h2.client_nonce = ::io::h2nl(client_nonce);
                h2.mtu          = ::io::h2ns(desired_mtu);
                h2.features     = ::io::h2ns(desired_features);

                (void)send_to_peer(ps.ep, MSG_HELLO2, UdpChan::Reliable,
                    byte_view{ (u8*)&h2, sizeof(h2) }, now_ms);

                ps.hs = udp_peer_state::HS_COOKIE_SENT;
                ps.hs_deadline_ms = now_ms + HANDSHAKE_TIMEOUT_MS;
                return true;
            }

            if (type == MSG_WELCOME) {
                if (payload.size() != sizeof(msg_welcome)) return false;
                const bool was_established = (ps.hs == udp_peer_state::HS_ESTABLISHED);
                msg_welcome w{};
                for (usize i = 0; i < sizeof(w); ++i) ((u8*)&w)[i] = payload.data()[i];

                ps.mtu        = clamp_mtu(::io::n2hs(w.mtu));
                ps.features   = ::io::n2hs(w.features);
                ps.session_id = ::io::n2hl(w.session_id);
                ps.hs = udp_peer_state::HS_ESTABLISHED;

                session_id = ps.session_id; // store in loop

                if (!was_established && cb.on_established)
                    cb.on_established(cb.ud, ps.ep, ps.session_id);
                secure_zero(&client_nonce, sizeof(client_nonce));
                return true;
            }

            return false;
        }
        bool start_client_handshake(Endpoint server, u16 mtu, u16 feats, u64 now_ms) noexcept {
            is_server = false;
            server_ep = server;
            (void)os_entropy(&client_nonce, sizeof(client_nonce));
            if (client_nonce == 0) client_nonce = 946930; // avoid zero
            desired_mtu = clamp_mtu(mtu);
            desired_features = feats;

            // ensure peer exists in table BEFORE start_client_handshake()
            udp_peer_state* ps = find_peer(server_ep);
            if (!ps) return false;

            msg_hello h{};
            h.mtu          = ::io::h2ns(desired_mtu);
            h.features     = ::io::h2ns(desired_features);
            h.client_nonce = ::io::h2nl(client_nonce);

            ps->hs = udp_peer_state::HS_NONE;
            ps->hs_deadline_ms = now_ms + HANDSHAKE_TIMEOUT_MS;

            return send_to_peer(server_ep, MSG_HELLO, UdpChan::Unreliable,
                byte_view((u8*)&h, sizeof(h)), now_ms);
        }

    private:
        static inline void drop(UdpCallbacks& cb, Endpoint from, Error why, DropReason r) noexcept {
            if (cb.on_drop) cb.on_drop(cb.ud, from, why, r);
        }

        static bool hs_payload_ok(u8 t, int payload_len) noexcept {
            if (t == MSG_HELLO)  return payload_len == (int)sizeof(msg_hello);
            if (t == MSG_HELLO2) return payload_len == (int)sizeof(msg_hello2);
            if (t == MSG_COOKIE || t == MSG_WELCOME) return false;
            return false;
        };

    public:

#if defined(_WIN32)
        Socket* items[MAX_PEERS]{};
        int count = 0;
        void add(Socket& s) noexcept { items[count++] = &s; }

        void run_udp(Socket& udp, UdpCallbacks& cb, u8* recv_buf, int recv_cap) noexcept {
            add(udp);
            cb_live = &cb;

            while (running) {
                WSAPOLLFD fds[MAX_PEERS]{};
                for (int i = 0; i < count; ++i) {
                    fds[i].fd = items[i]->native();
                    fds[i].events = POLLRDNORM; // read only; avoid always-writable spin
                    fds[i].revents = 0;
                }

                // Wake periodically so tick_sessions()/tick_reliable() can run and clean peers.
                const int timeout_ms = sendq.empty() ? 10 : 0;
                (void)WSAPoll(fds, (ULONG)count, timeout_ms);

                // 1) send queued refs (arena-backed)
                const u64 now_ms = monotonic_ms();
                flush_outgoing(udp);        
                tick_reliable(now_ms);
                tick_sessions(udp, cb, now_ms);

                // 2) drain receives
                if (fds[0].revents & POLLRDNORM) {
                    for (;;) {
                        Endpoint from{};
                        const int n = udp.recv_from(from, { recv_buf, (usize)recv_cap });
                        if (n <= 0) break;

                        if (n < (int)sizeof(UdpHeader)) {
                            drop(cb, from, Error::Generic, DropReason::TooSmall);
                            continue;
                        }

                        // Safer on non-x86: copy header out (avoid unaligned access).
                        UdpHeader h{};
                        const u8* src = recv_buf;
                        u8* dst = (u8*)&h;
                        for (usize k = 0; k < sizeof(UdpHeader); ++k) dst[k] = src[k];

                        udp_header_wire_to_host(h);

                        if (h.magic != UDP_MAGIC) {
                            drop(cb, from, Error::Generic, DropReason::BadMagic);
                            continue;
                        }
                        if (h.version != UDP_VERSION) {
                            drop(cb, from, Error::Generic, DropReason::BadVer);
                            continue;
                        }

                        const int payload_len = (int)h.payload_len;
                        if (payload_len < 0 || (int)sizeof(UdpHeader) + payload_len != n) {
                            drop(cb, from, Error::Generic, DropReason::BadLen);
                            continue;
                        }

                        const byte_view payload_view{ recv_buf + sizeof(UdpHeader), (usize)payload_len };

                        // handshake messages allowed always
                        if (is_handshake_msg(h.type)) {
                            if (!hs_payload_ok(h.type, payload_len)) {
                                drop(cb, from, Error::Generic, DropReason::BadHs);
                                continue;
                            }
                            udp_peer_state* ps = get_peer_create(from);
                            if (!ps) {
                                drop(cb, from, Error::Generic, DropReason::FullPeerTable);
                                continue;
                            }
                            ps->last_rx_ms = now_ms; // just received something (put now_ms earlier)
                            ps->remote_ack = h.ack;
                            ps->remote_ack_bits = h.ack_bits;
                            on_remote_ack(*ps);

                            const bool ok_hs = is_server
                                ? handle_handshake_server(*ps, cb, h.type, payload_view, now_ms)
                                : handle_handshake_client(*ps, cb, h.type, payload_view, now_ms);
                            // if handshake msg invalid -> drop
                            if (ok_hs) continue;

                            // If already established, treat stray HS as noise (no drop spam)
                            if (ps->hs == udp_peer_state::HS_ESTABLISHED) {
                                continue;
                            } 

                            drop(cb, from, Error::Generic, DropReason::BadHs);
                            continue;
                        }

                        // ---------------- non-handshake path: must NOT CREATE peer ----------------
                        udp_peer_state* ps = find_peer(from);
                        if (!ps) {
                            // Unknown sender sending non-handshake => ignore silently.
                            continue;
                        }
                        ps->last_rx_ms = now_ms;
                        ps->remote_ack = h.ack;
                        ps->remote_ack_bits = h.ack_bits;
                        on_remote_ack(*ps);


                        // keepalive allowed only if established
                        if (h.type == MSG_KEEPALIVE) {
                            if (ps->hs != udp_peer_state::HS_ESTABLISHED) continue;
                            // optional: validate session_id from payload
                            continue;
                        }

                        // allow DISCONNECT control packet from known peer
                        if (h.type == MSG_DISCONNECT) {
                            // ignore if not established
                            if (ps->hs != udp_peer_state::HS_ESTABLISHED) continue; 
                            if (payload_len != (int)sizeof(msg_disconnect)) {
                                drop(cb, from, Error::Generic, DropReason::BadCtrl);
                                continue;
                            }
                            msg_disconnect d{};
                            for (usize i = 0; i < sizeof(d); ++i) ((u8*)&d)[i] = payload_view.data()[i];

                            const u32 sid = ::io::n2hl(d.session_id);
                            if (sid != ps->session_id) {
                                drop(cb, from, Error::Generic, DropReason::BadCtrl);
                                continue;
                            }
                            handle_disconnect(cb, *ps, (DisconnectReason)d.reason);
                            continue;
                        }


                        // for any user packets: require established
                        if (ps->hs != udp_peer_state::HS_ESTABLISHED) {
                            continue;
                        }

                        // MTU contract: drop if header says payload_len exceeds negotiated cap
                        const u16 cap = mtu_payload_cap(ps->mtu);
                        if ((u32)payload_len > (u32)cap) {
                            drop(cb, from, Error::Generic, DropReason::BadMtu);
                            continue;
                        }

                        if (!ps->recv_win.accept(h.seq)) {
                            continue; // dup/too old
                        }

                        if (cb.on_packet) {
                            cb.on_packet(cb.ud, from, h.type, (UdpChan)h.chan, payload_view);
                        }
                    }
                }
            }
        }

#elif defined(__linux__)
        int ep{ -1 }; // MUST be initialized with `-1`

        IO_NODISCARD bool init_epoll() noexcept {
            const long r = native::sys_epoll_create1(0);
            if (native::is_err(r)) return false;
            ep = (int)r;
            return true;
        }

        IO_NODISCARD bool add(Socket& s) noexcept {
            using namespace native;
            if (ep < 0) return false;
            epoll_event ev{};
            ev.events = k_epollin; // read only; avoid always-writable spin
            ev.data.fd = (int)s.native();
            const long r = sys_epoll_ctl(ep, k_epoll_ctl_add, (int)s.native(), &ev);
            return !is_err(r);
        }

        void run_udp(Socket& udp, UdpCallbacks& cb,
                     u8* recv_buf, int recv_cap) noexcept
        {
            if (ep < 0 && !init_epoll()) return;
            (void)add(udp);
            cb_live = &cb;

            native::epoll_event events[16]{};

            while (running) {
                const int timeout_ms = sendq.empty() ? 10 : 0;
                const long n_ev_l = native::sys_epoll_wait(ep, events, 16, timeout_ms);
                int n_ev = 0;
                if (native::is_err(n_ev_l)) {
                    if (native::err_no(n_ev_l) == native::k_eintr) n_ev = 0;
                    else n_ev = 0;
                } else {
                    n_ev = (int)n_ev_l;
                }

                // 1) send queued refs (arena-backed)
                const u64 now_ms = monotonic_ms();
                flush_outgoing(udp);
                tick_reliable(now_ms);
                tick_sessions(udp, cb, now_ms);

                // 2) drain receives
                for (int i = 0; i < n_ev; ++i) {
                    if ((events[i].events & native::k_epollin) == 0) continue;

                    for (;;) {
                        Endpoint from{};
                        const int n = udp.recv_from(from, recv_buf, recv_cap);
                        if (n <= 0) break;

                        if (n < (int)sizeof(UdpHeader)) {
                            drop(cb, from, Error::Generic, DropReason::TooSmall);
                            continue;
                        }

                        // Safer on non-x86: copy header out (avoid unaligned access).
                        UdpHeader h{};
                        const u8* src = recv_buf;
                        u8* dst = (u8*)&h;
                        for (usize k = 0; k < sizeof(UdpHeader); ++k) dst[k] = src[k];

                        udp_header_wire_to_host(h);

                        if (h.magic != UDP_MAGIC) {
                            drop(cb, from, Error::Generic, DropReason::BadMagic);
                            continue;
                        }
                        if (h.version != UDP_VERSION) {
                            drop(cb, from, Error::Generic, DropReason::BadVer);
                            continue;
                        }

                        const int payload_len = (int)h.payload_len;
                        if (payload_len < 0 || (int)sizeof(UdpHeader) + payload_len != n) {
                            drop(cb, from, Error::Generic, DropReason::BadLen);
                            continue;
                        }

                        // handshake messages allowed always
                        const byte_view payload_view{ recv_buf + sizeof(UdpHeader), (usize)payload_len };

                        if (is_handshake_msg(h.type)) {
                            if (!hs_payload_ok(h.type, payload_len)) {
                                drop(cb, from, Error::Generic, DropReason::BadHs);
                                continue;
                            }
                            udp_peer_state* ps = get_peer_create(from);
                            if (!ps) {
                                drop(cb, from, Error::Generic, DropReason::FullPeerTable);
                                continue;
                            }
                            ps->last_rx_ms = now_ms; // just received something (put now_ms earlier)
                            ps->remote_ack = h.ack;
                            ps->remote_ack_bits = h.ack_bits;
                            on_remote_ack(*ps);

                            const bool ok_hs = is_server
                                ? handle_handshake_server(*ps, cb, h.type, payload_view, now_ms)
                                : handle_handshake_client(*ps, cb, h.type, payload_view, now_ms);
                            if (ok_hs) continue;
                            
                            // If already established, treat stray HS as noise (no drop spam)
                            if (ps->hs == udp_peer_state::HS_ESTABLISHED)
                                continue;
                            drop(cb, from, Error::Generic, DropReason::BadHs);
                            continue;
                        }

                        // ---------------- non-handshake path: must NOT CREATE peer ----------------
                        udp_peer_state* ps = find_peer(from);
                        if (!ps) {
                            // Unknown sender sending non-handshake => ignore silently.
                            continue;
                        }
                        ps->last_rx_ms = now_ms;
                        ps->remote_ack = h.ack;
                        ps->remote_ack_bits = h.ack_bits;
                        on_remote_ack(*ps);

                        // keepalive allowed only if established
                        if (h.type == MSG_KEEPALIVE) {
                            if (ps->hs != udp_peer_state::HS_ESTABLISHED) continue;
                            // optional: validate session_id from payload
                            continue;
                        }

                        // allow DISCONNECT control packet from known peer
                        if (h.type == MSG_DISCONNECT) {
                            // ignore if not established
                            if (ps->hs != udp_peer_state::HS_ESTABLISHED) continue; 
                            if (payload_len != (int)sizeof(msg_disconnect)) {
                                drop(cb, from, Error::Generic, DropReason::BadCtrl);
                                continue;
                            }
                            msg_disconnect d{};
                            for (usize i = 0; i < sizeof(d); ++i) ((u8*)&d)[i] = payload_view.data()[i];

                            const u32 sid = ::io::n2hl(d.session_id);
                            if (sid != ps->session_id) {
                                drop(cb, from, Error::Generic, DropReason::BadCtrl);
                                continue;
                            }
                            handle_disconnect(cb, *ps, (DisconnectReason)d.reason);
                            continue;
                        }

                        // for any user packets: require established
                        if (ps->hs != udp_peer_state::HS_ESTABLISHED) continue;

                        // MTU contract: drop if header says payload_len exceeds negotiated cap
                        const u16 cap = mtu_payload_cap(ps->mtu);
                        if ((u32)payload_len > (u32)cap) {
                            drop(cb, from, Error::Generic, DropReason::BadMtu);
                            continue;
                        }

                        if (!ps->recv_win.accept(h.seq)) {
                            continue; // dup/too old
                        }

                        if (cb.on_packet) {
                            cb.on_packet(cb.ud, from, h.type, (UdpChan)h.chan,
                                byte_view{ recv_buf + sizeof(UdpHeader), (usize)payload_len });
                        }
                    }
                }
            }
        }
#endif
    }; // struct EventLoop

} // namespace io


inline const io::Out& operator<<(const io::Out& t, const io::IP& ip) noexcept {
#ifdef IO_TERMINAL
    io::u32 h = io::n2hl(ip.addr_be);
    t << ((h >> 24) & 0xFFu) << '.'
      << ((h >> 16) & 0xFFu) << '.'
      << ((h >> 8 ) & 0xFFu) << '.'
      << ((h >> 0 ) & 0xFFu);
#endif
    return t;
}

inline const io::Out& operator<<(const io::Out& t, const io::Endpoint& ep) noexcept {
#ifdef IO_TERMINAL
    t << io::IP(ep.addr_be) << ':' << (io::u32)io::n2hs(ep.port_be);
#endif
    return t;
}

inline const io::Out& operator<<(const io::Out& t, io::DropReason dr) noexcept {
#ifdef IO_TERMINAL
    t << io::drop_reason_str(dr);
#endif
    return t;
}

inline const io::Out& operator<<(const io::Out& t, io::DisconnectReason dr) noexcept {
#ifdef IO_TERMINAL
    t << io::disconnect_reason_str(dr);
#endif
    return t;
}

inline const io::Out& operator<<(const io::Out& t, io::Protocol proto) noexcept {
#ifdef IO_TERMINAL
    t << (proto==io::Protocol::TCP ? "TCP" : "UDP");
#endif
    return t;
}

inline const io::Out& operator<<(const io::Out& t, io::Error err) noexcept {
#ifdef IO_TERMINAL
    t << io::error_str(err);
#endif
    return t;
}