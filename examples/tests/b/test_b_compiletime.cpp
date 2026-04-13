#include "../../../b/b.hpp"

static_assert(sizeof(b::thread_id) >= sizeof(a::u32), "b::thread_id must carry a stable integral id");
static_assert(sizeof(b::socket_handle) >= sizeof(a::isize), "b::socket_handle must store native-sized token");
static_assert(static_cast<int>(b::socket_status::ok) == 0, "socket_status ABI must remain stable");

int main() {
    return 0;
}

