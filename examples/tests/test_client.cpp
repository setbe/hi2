// Build: Debug/Release with std.
// Run: Run `server.exe`, and only then `test_client.exe`

#define CATCH_CONFIG_MAIN
#define IO_IMPLEMENTATION
#include "test_socket.hpp"

TEST_CASE("Attack phases: do not elicit COOKIE/WELCOME; server still handshakes after", "[security]") {
    if (!test_socket::external_server_ready()) {
        WARN("Skipping: start server.exe first (test_server.exe is not used here).");
        return;
    }

    Endpoint srv = test_socket::server_ep();

    Socket attacker = test_socket::make_udp_socket();

    constexpr u32 BUF_CAP = 4096;
    u8 tx[BUF_CAP]{};
    u8 rx[BUF_CAP]{};
    u32 rng = 0xC0FFEEu;

    // -------------------- Phase 1: tiny garbage (< header) --------------------
    for (u32 i = 0; i < 300; ++i) {
        const u32 n = (rng % 25u); // 0..24 (< 26)
        test_socket::fill_prng(rng, tx, n);
        if (n) (void)test_socket::send_raw(attacker, srv, tx, n);
    }
    test_socket::require_no_cookie_or_welcome(attacker, srv, rx, BUF_CAP, /*window_ms=*/50);

    // -------------------- Phase 2: wrong magic/version + framing mismatches --------------------
    {
        msg_hello h{};
        test_socket::build_hello(h, 1200, FEATURE_COOKIE, 0x11111111u);

        // wrong magic (well-framed)
        {
            const u32 n = test_socket::build_udp_datagram(
                tx, BUF_CAP, 0xDEADBEEFu, UDP_VERSION,
                10, 0, 0, (u8)UdpChan::Unreliable, MSG_HELLO,
                (const u8*)&h, (u16)sizeof(h), (u32)sizeof(h)
            );
            REQUIRE(n != 0);
            (void)test_socket::send_raw(attacker, srv, tx, n);
        }

        // wrong version (well-framed)
        {
            const u32 n = test_socket::build_udp_datagram(
                tx, BUF_CAP, UDP_MAGIC, (u16)(UDP_VERSION + 1),
                11, 0, 0, (u8)UdpChan::Unreliable, MSG_HELLO,
                (const u8*)&h, (u16)sizeof(h), (u32)sizeof(h)
            );
            REQUIRE(n != 0);
            (void)test_socket::send_raw(attacker, srv, tx, n);
        }

        // payload_len_in_header larger than actual bytes
        {
            const u32 n = test_socket::build_udp_datagram(
                tx, BUF_CAP, UDP_MAGIC, UDP_VERSION,
                12, 0, 0, (u8)UdpChan::Unreliable, MSG_HELLO,
                (const u8*)&h, (u16)(sizeof(h) + 7), (u32)sizeof(h)
            );
            REQUIRE(n != 0);
            (void)test_socket::send_raw(attacker, srv, tx, n);
        }

        // payload_len_in_header smaller than actual bytes
        {
            u8 padded[sizeof(h) + 8]{};
            for (u32 i = 0; i < (u32)sizeof(h); ++i) padded[i] = ((u8*)&h)[i];
            test_socket::fill_prng(rng, padded + sizeof(h), 8);

            const u32 n = test_socket::build_udp_datagram(
                tx, BUF_CAP, UDP_MAGIC, UDP_VERSION,
                13, 0, 0, (u8)UdpChan::Unreliable, MSG_HELLO,
                padded, (u16)sizeof(h), (u32)(sizeof(h) + 8)
            );
            REQUIRE(n != 0);
            (void)test_socket::send_raw(attacker, srv, tx, n);
        }

        // handshake type but wrong payload size (empty) — should not elicit COOKIE/WELCOME
        {
            const u32 n = test_socket::build_udp_datagram(
                tx, BUF_CAP, UDP_MAGIC, UDP_VERSION,
                14, 0, 0, (u8)UdpChan::Unreliable, MSG_HELLO,
                nullptr, 0, 0
            );
            REQUIRE(n != 0);
            (void)test_socket::send_raw(attacker, srv, tx, n);
        }
    }
    test_socket::require_no_cookie_or_welcome(attacker, srv, rx, BUF_CAP, /*window_ms=*/120);

    // -------------------- Phase 3: user packet before establish should be ignored --------------------
    {
        u8 payload[16]{};
        test_socket::fill_prng(rng, payload, 16);

        const u32 n = test_socket::build_udp_datagram(
            tx, BUF_CAP,
            UDP_MAGIC, UDP_VERSION,
            20, 0, 0,
            (u8)UdpChan::Reliable, /*type=*/32,
            payload, /*payload_len_in_header=*/16, /*payload_bytes_to_copy=*/16
        );
        REQUIRE(n != 0);
        (void)test_socket::send_raw(attacker, srv, tx, n);
    }
    test_socket::require_no_cookie_or_welcome(attacker, srv, rx, BUF_CAP, /*window_ms=*/120);

    // -------------------- Final: honest handshake from a fresh socket must succeed --------------------
    Socket honest = test_socket::make_udp_socket();

    REQUIRE(test_socket::honest_handshake(
        honest, srv,
        /*nonce=*/0x12345678u,
        /*mtu=*/1200,
        /*feats=*/FEATURE_COOKIE,
        /*per_step_timeout_ms=*/1500,
        /*hello_retries=*/5
    ));
}

TEST_CASE("Peer table pressure: after filling > MAX_PEERS with invalid HELLO payloads, server still handshakes later", "[security]") {
    if (!test_socket::external_server_ready()) {
        WARN("Skipping: start server.exe first (test_server.exe is not used here).");
        return;
    }

    Endpoint srv = test_socket::server_ep();

    constexpr u32 N = 80; // > 64
    Socket socks[N]{};

    constexpr u32 BUF_CAP = 256;
    u8 tx[BUF_CAP]{};

    for (u32 i = 0; i < N; ++i) {
        socks[i] = test_socket::make_udp_socket();

        const u32 n = test_socket::build_udp_datagram(
            tx, BUF_CAP,
            UDP_MAGIC, UDP_VERSION,
            /*seq=*/1, 0, 0,
            (u8)UdpChan::Unreliable, MSG_HELLO,
            nullptr, /*payload_len_in_header=*/0, /*payload_bytes_to_copy=*/0
        );
        REQUIRE(n != 0);
        (void)test_socket::send_raw(socks[i], srv, tx, n);
    }

    io::sleep_ms(HANDSHAKE_TIMEOUT_MS + 250);

    Socket honest = test_socket::make_udp_socket();
    REQUIRE(test_socket::honest_handshake(
        honest, srv,
        /*nonce=*/0x13572468u,
        /*mtu=*/1200,
        /*feats=*/FEATURE_COOKIE,
        /*per_step_timeout_ms=*/2000,
        /*hello_retries=*/6
    ));
}
