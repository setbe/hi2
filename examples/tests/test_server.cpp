// Build: Debug/Release with std.
// Run: Run `test_client.exe` WITHOUT running `server.exe`

#define CATCH_CONFIG_MAIN
#define IO_IMPLEMENTATION
#include "test_socket.hpp"

#include <thread>
#include <atomic>
#include <vector>

using namespace io;

IO_CONSTEXPR view<const u8> vec2view(const std::vector<u8>& v) noexcept { return view<const u8>{ v.data(), v.size() }; }

using Loop = EventLoop<1200, 2048>;

struct Counters {
    std::atomic<u32> established{ 0 };
    std::atomic<u32> drops_total{0};
    std::atomic<u32> drops_reason[9]{}; // 0 unused
    std::atomic<u32> user_pkts{ 0 };
    std::atomic<u32> disconnects{ 0 };
    std::atomic<u32> last_reason{ 0 };
};

static void cb_established(void* ud, Endpoint, u32) {
    ((Counters*)ud)->established.fetch_add(1, std::memory_order_relaxed);
}
static void cb_drop(void* ud, Endpoint, Error, DropReason r) {
    auto* c = (Counters*)ud;
    c->drops_total.fetch_add(1, std::memory_order_relaxed);
    const u32 idx = (u32)r;
    if (idx < 8) c->drops_reason[idx].fetch_add(1, std::memory_order_relaxed);
}
static void cb_packet(void* ud, Endpoint, u8 type, UdpChan, byte_view) {
    if (type >= 32)
        ((Counters*)ud)->user_pkts.fetch_add(1, std::memory_order_relaxed);
}

static void cb_disconnect(void* ud, Endpoint, u32, DisconnectReason why) {
    auto* c = (Counters*)ud;
    c->last_reason.store((u32)why, std::memory_order_relaxed);
    c->disconnects.fetch_add(1, std::memory_order_relaxed);
}

static std::vector<u8> make_datagram(
    u32 magic, u16 version,
    u8 type, u8 chan,
    u32 seq, u32 ack, u64 ack_bits,
    const u8* payload, u16 payload_len)
{
    UdpHeader h{};
    h.ack_bits = ack_bits;
    h.magic = magic;
    h.seq = seq;
    h.ack = ack;
    h.version = version;
    h.payload_len = payload_len;
    h.chan = chan;
    h.type = type;

    udp_header_host_to_wire(h);

    std::vector<u8> out;
    out.resize(sizeof(UdpHeader) + payload_len);
    u8* dst = out.data();
    const u8* hs = (const u8*)&h;
    for (u32 i = 0; i < (u32)sizeof(UdpHeader); ++i) dst[i] = hs[i];
    for (u32 i = 0; i < payload_len; ++i) dst[sizeof(UdpHeader) + i] = payload ? payload[i] : 0;
    return out;
}

static std::vector<u8> make_disconnect(u32 sid, DisconnectReason r,
    u32 seq = 1, u32 ack = 0, u64 ack_bits = 0)
{
    msg_disconnect d{};
    d.session_id = io::h2nl(sid);
    d.reason = (u8)r;

    return make_datagram(UDP_MAGIC, UDP_VERSION,
        MSG_DISCONNECT, (u8)UdpChan::Reliable,
        seq, ack, ack_bits,
        (const u8*)&d, (u16)sizeof(d));
}

static void spin_sleep_ms(int ms) {
    // simple sleep wrapper (use std::this_thread::sleep_for in hosted tests)
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

TEST_CASE("server drops malformed packets and never establishes", "[udp][fuzz]") {
    Socket server{};
    REQUIRE(server.open(Protocol::UDP));
    Endpoint bind{};
    bind.addr_be = IP::from_string("127.0.0.1");
    bind.port_be = io::h2ns(test_socket::SERVER_PORT);
    REQUIRE(server.bind(bind));
    (void)server.set_blocking(false);

    Loop loop{};
    REQUIRE(loop.init(true));

    constexpr usize RECV_CAP = 4096;
    unique_bytes recv_buf{ (u8*)alloc_aligned(RECV_CAP, 16) };
    REQUIRE(recv_buf.get() != nullptr);

    Counters ctr{};
    UdpCallbacks cb{};
    cb.on_established = &cb_established;
    cb.on_drop = &cb_drop;
    cb.on_packet = &cb_packet;
    cb.on_disconnect = &cb_disconnect;
    cb.ud = &ctr;

    std::thread thr([&] {
        loop.run_udp(server, cb, recv_buf.get(), RECV_CAP);
    });

    Socket atk{};
    REQUIRE(atk.open(Protocol::UDP));
    Endpoint atk_bind{};
    atk_bind.addr_be = IP::from_string("127.0.0.1");
    atk_bind.port_be = io::h2ns(0);
    REQUIRE(atk.bind(atk_bind));
    (void)atk.set_blocking(false);

    Endpoint to{};
    to.addr_be = IP::from_string("127.0.0.1");
    to.port_be = io::h2ns(test_socket::SERVER_PORT);

    constexpr io::u64 TIMEOUT_MS = 100;
    // 1) too short
    {
        u8 tiny[8]{};

        const int sent = atk.send_to(to, as_view(tiny).v);
        INFO("sent=" << sent << " err=" << atk.error_str().data());
        REQUIRE(sent == 8);
    }

    // 2) bad magic
    {
        auto d = make_datagram(0xDEADBEEFu, UDP_VERSION, MSG_HELLO,
                (u8)UdpChan::Unreliable, 1, 0, 0, nullptr, 0);

        const int sent = atk.send_to(to, vec2view(d));
        INFO("sent=" << sent << " err=" << atk.error_str().data());
        REQUIRE(sent == d.size());
    }

    // 3) bad version
    {
        auto d = make_datagram(UDP_MAGIC, 0x9999u, MSG_HELLO,
                (u8)UdpChan::Unreliable, 2, 0, 0, nullptr, 0);

        const int sent = atk.send_to(to, vec2view(d));
        INFO("sent=" << sent << " err=" << atk.error_str().data());
        REQUIRE(sent == d.size());
    }

    // 4) payload_len mismatch (claims 10, actually 0)
    {
        auto d = make_datagram(UDP_MAGIC, UDP_VERSION, MSG_HELLO,
                (u8)UdpChan::Unreliable, 3, 0, 0, nullptr, 10);

        // send only header bytes, truncated
        const int sent = atk.send_to(to, vec2view(d).subview(0, sizeof(UdpHeader)));
        INFO("sent=" << sent << " err=" << atk.error_str().data());
        REQUIRE(sent == (int)sizeof(UdpHeader));
    }

    // 5) user packet before establish should be ignored (no user_pkts++)
    {
        u8 p[4]{ 'A','B','C','D' };
        auto d = make_datagram(UDP_MAGIC, UDP_VERSION, 32, (u8)UdpChan::Reliable,
            4, 0, 0, p, 4);

        const int sent = atk.send_to(to, vec2view(d));
        INFO("sent=" << sent << " err=" << atk.error_str().data());
        REQUIRE(sent == d.size());
    }

    // send DISCONNECT before any valid handshake => peer not registered => MUST NOT call on_disconnect
    {
        auto d = make_disconnect(/*sid=*/1, DisconnectReason::Unknown, /*seq=*/9);

        const int sent = atk.send_to(to, vec2view(d));
        INFO("sent=" << sent << " err=" << atk.error_str().data());
        REQUIRE(sent == d.size());
    }

    spin_sleep_ms(200);

    CHECK(ctr.established.load() == 0);
    CHECK(ctr.user_pkts.load() == 0);
    CHECK(ctr.drops_total.load() >= 1); // at least short/bad packets should count as drops
    CHECK(ctr.disconnects.load() == 0);
    CHECK(ctr.drops_reason[(u32)DropReason::TooSmall].load() >= 1);
    CHECK(ctr.drops_reason[(u32)DropReason::BadMagic].load() >= 1);

    loop.stop();
    thr.join();
}



TEST_CASE("server does not accept HELLO2 without valid COOKIE", "[udp][handshake]") {
    INFO("Implement TU"); // TODO
}



TEST_CASE("registered peer: DISCONNECT triggers callback", "[udp][disconnect]") {
    Socket server{};
    REQUIRE(server.open(Protocol::UDP));
    Endpoint bind{};
    bind.addr_be = IP::from_string("127.0.0.1");
    bind.port_be = io::h2ns(test_socket::SERVER_PORT);
    REQUIRE(server.bind(bind));
    (void)server.set_blocking(false);

    Loop loop{};
    REQUIRE(loop.init(true));

    constexpr usize RECV_CAP = 4096;
    unique_bytes recv_buf{ (u8*)alloc_aligned(RECV_CAP, 16) };
    REQUIRE(recv_buf.get() != nullptr);

    Counters ctr{};
    UdpCallbacks cb{};
    cb.on_established = &cb_established;
    cb.on_drop = &cb_drop;
    cb.on_packet = &cb_packet;
    cb.on_disconnect = &cb_disconnect;
    cb.ud = &ctr;

    std::thread thr([&] { loop.run_udp(server, cb, recv_buf.get(), RECV_CAP); });

    // honest client socket (raw handshake)
    Socket cli{};
    REQUIRE(cli.open(Protocol::UDP));
    Endpoint cli_bind{};
    cli_bind.addr_be = IP::from_string("127.0.0.1");
    cli_bind.port_be = io::h2ns(0);
    REQUIRE(cli.bind(cli_bind));
    (void)cli.set_blocking(false);

    Endpoint srv{};
    srv.addr_be = IP::from_string("127.0.0.1");
    srv.port_be = io::h2ns(test_socket::SERVER_PORT);

    // complete handshake (must establish)
    REQUIRE(test_socket::honest_handshake(cli, srv, 0x12345678u, 1200, FEATURE_COOKIE, 1500, 5));
    sleep_ms(50);
    REQUIRE(ctr.established.load() >= 1);

    // now send DISCONNECT with correct sid=1 (in your server it increments from 1)
    auto d = make_disconnect(/*sid=*/1, DisconnectReason::LocalReset, /*seq=*/500);
    REQUIRE(cli.send_to(srv, vec2view(d)) == (int)d.size());

    sleep_ms(100);

    CHECK(ctr.disconnects.load() == 1);
    CHECK(ctr.last_reason.load() == (u32)DisconnectReason::LocalReset);

    loop.stop();
    thr.join();
}


