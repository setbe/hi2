#define IO_IMPLEMENTATION
#include "../hi/socket.hpp"

using Loop = io::EventLoop<1200, 2048>;

static io::Endpoint g_server{};

static void on_established(void* ud, io::Endpoint peer, io::u32 sid) {
    auto* loop = (Loop*)ud;

    io::out << "ESTABLISHED peer=" << peer << " sid=" << sid << '\n';

    static const io::u8 ping[] = { 'P','I','N','G' };
    const bool ok = loop->send_to_peer(
        peer,
        32, // user msg type
        io::UdpChan::Reliable,
        io::byte_view(ping, sizeof(ping)),
        io::monotonic_ms()
    );
    if (!ok) io::out << "send ping failed\n";
}

static void on_packet(void*, io::Endpoint from, io::u8 type, io::UdpChan, io::byte_view payload) {
    if (!io::endpoint_eq(from, g_server)) return;

    if (type == 32) {
        io::out << "ECHO: " << payload << '\n';
    }
}

static void on_drop(void*, io::Endpoint from, io::Error why, io::DropReason dr) {
    io::out << "DROP from " << from << " err=" << why << " reason=" << dr << '\n';
}

static void on_disconnect(void*, io::Endpoint peer, io::u32 /*session_id*/, io::DisconnectReason why) {
    io::out << "DISCONNECT peer=" << peer << " why=" << why << '\n';
}


int main() {
    io::sleep_ms(1000);

    g_server.addr_be = io::IP::from_string("127.0.0.1");
    g_server.port_be = io::h2ns(7777);

    io::Socket udp{};
    if (!udp.open(io::Protocol::UDP)) return 1;

    io::Endpoint bind_ep{};
    bind_ep.addr_be = io::IP::from_string("0.0.0.0");
    bind_ep.port_be = io::h2ns(0);
    if (!udp.bind(bind_ep)) return 2;
    (void)udp.set_blocking(false);

    Loop loop{};
    if (!loop.init(/*is_server=*/false)) return 3;

    // ensure peer exists in table BEFORE start_client_handshake()
    if (!loop.get_peer_create(g_server)) return 4;

    // start handshake
    {
        const io::u64 now = io::monotonic_ms();
        if (!loop.start_client_handshake(g_server, 1200, io::FEATURE_COOKIE, now))
            return 5;
    }

    constexpr io::usize RECV_BUF_SIZE = 2048;
    io::unique_bytes recv_buf{ (io::u8*)io::alloc(RECV_BUF_SIZE) };
    if (!recv_buf.get()) return 6;

    io::UdpCallbacks cb{};
    cb.on_packet = &on_packet;
    cb.on_drop = &on_drop;
    cb.on_established = &on_established;
    cb.on_disconnect = &on_disconnect;
    cb.ud = &loop;

    io::out << "client started\n";
    loop.run_udp(udp, cb, recv_buf.get(), RECV_BUF_SIZE);
    return 0;
}
