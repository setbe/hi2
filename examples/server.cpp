#define IO_IMPLEMENTATION
#include "../hi/socket.hpp"

using Loop = io::EventLoop<1200, 2048>;

static void on_established(void*, io::Endpoint peer, io::u32 sid) {
    io::out << "ESTABLISHED peer=" << peer << " sid=" << sid << '\n';
}

static void on_packet(void* ud, io::Endpoint from, io::u8 type,
                      io::UdpChan chan, io::byte_view payload) {
    auto* loop = (Loop*)ud;

    io::out << "PKT type=" << (io::u32)type
        << " chan=" << (io::u32)chan
        << " bytes=" << (io::u32)payload.size()
        << " from " << from << '\n';

    // user packets only (example: type>=32)
    if (type < 32) return;

    auto* ps = loop->find_peer(from);
    if (!ps || ps->hs != Loop::udp_peer_state::HS_ESTABLISHED) return;

    (void)loop->send_to_peer(from, type, chan, payload, io::monotonic_ms());
}

static void on_drop(void*, io::Endpoint from, io::Error why, io::DropReason dr) {
    io::out << "DROP from " << from << " err=" << why << " reason=" << dr << '\n';
}

static void on_disconnect(void*, io::Endpoint peer, io::u32 /*session_id*/, io::DisconnectReason why) {
    io::out << "DISCONNECT peer=" << peer << " why=" << why << '\n';
}


int main() {
    io::Socket udp{};
    if (!udp.open(io::Protocol::UDP)) return 1;

    io::Endpoint bind_ep{};
    bind_ep.addr_be = io::IP::from_string("0.0.0.0");
    bind_ep.port_be = io::h2ns(7777);
    if (!udp.bind(bind_ep)) return 2;
    (void)udp.set_blocking(false);

    Loop loop{};
    if (!loop.init(/*is_server=*/true)) return 3;

    constexpr io::usize RECV_BUF_SIZE = 2048;
    io::unique_bytes recv_buf{ (io::u8*)io::alloc(RECV_BUF_SIZE)};
    if (!recv_buf.get()) return 4;

    io::UdpCallbacks cb{};
    cb.on_packet = &on_packet;
    cb.on_drop = &on_drop;
    cb.on_established = &on_established;
    cb.on_disconnect = &on_disconnect;
    cb.ud = &loop;

    io::out << "server started on :7777\n";
    loop.run_udp(udp, cb, recv_buf.get(), RECV_BUF_SIZE);
    return 0;
}
