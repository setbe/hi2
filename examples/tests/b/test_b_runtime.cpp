#include "../../../b/b.hpp"

int main() {
#if B_HAS_TIME
    const a::u64 t0 = b::monotonic_now_ns();
    b::sleep_ms(1);
    const a::u64 t1 = b::monotonic_now_ns();
    if (t1 < t0) {
        return 1;
    }
#endif

#if B_HAS_THREADS
    const b::thread_id tid = b::thread_current();
    if (tid.value == 0) {
        return 2;
    }
    b::thread_yield();
#endif

#if B_HAS_SOCKETS
    b::udp_socket s{};
    if (!s.open()) {
        return 3;
    }
    s.close();
#endif

    return 0;
}

