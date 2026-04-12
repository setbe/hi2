#pragma once

#include "config.hpp"
#include "types.hpp"

namespace a {
struct memory_req {
    usize bytes = 0;
    usize align = 1;
};

static A_CONSTEXPR_VAR usize req_overflow_bytes = static_cast<usize>(-1);

A_NODISCARD constexpr bool is_power_of_two(usize x) noexcept {
    return x != 0 && ((x & (x - 1)) == 0);
}

A_NODISCARD constexpr bool is_overflow(memory_req r) noexcept {
    return r.bytes == req_overflow_bytes;
}

A_NODISCARD constexpr bool is_valid(memory_req r) noexcept {
    return is_power_of_two(r.align) && !is_overflow(r);
}

A_NODISCARD constexpr usize align_up(usize value, usize align) noexcept {
    if (!is_power_of_two(align)) {
        return req_overflow_bytes;
    }
    const usize mask = align - 1;
    const usize aligned = (value + mask) & ~mask;
    if (aligned < value) {
        return req_overflow_bytes;
    }
    return aligned;
}

// req_max models peak reusable region (exclusive phases), not packed layout.
A_NODISCARD constexpr memory_req req_max(memory_req a, memory_req b) noexcept {
    if (is_overflow(a) || is_overflow(b) || !is_valid(a) || !is_valid(b)) {
        return memory_req{req_overflow_bytes, 1};
    }
    memory_req r{};
    r.bytes = (a.bytes > b.bytes) ? a.bytes : b.bytes;
    r.align = (a.align > b.align) ? a.align : b.align;
    return r;
}

// req_sum models sequential packed composition with alignment.
A_NODISCARD constexpr memory_req req_sum(memory_req a, memory_req b) noexcept {
    if (is_overflow(a) || is_overflow(b) || !is_valid(a) || !is_valid(b)) {
        return memory_req{req_overflow_bytes, 1};
    }

    const usize start_b = align_up(a.bytes, b.align);
    if (start_b == req_overflow_bytes) {
        return memory_req{req_overflow_bytes, (a.align > b.align) ? a.align : b.align};
    }
    if (b.bytes > (req_overflow_bytes - start_b)) {
        return memory_req{req_overflow_bytes, (a.align > b.align) ? a.align : b.align};
    }

    memory_req r{};
    r.bytes = start_b + b.bytes;
    r.align = (a.align > b.align) ? a.align : b.align;
    return r;
}
} // namespace a

