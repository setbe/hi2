#pragma once

#include "config.hpp"
#include "types.hpp"

namespace a {
enum class status : u8 {
    ok = 0,
    invalid_input,
    insufficient_memory,
    overflow,
    unsupported,
    backend_failure,
    mismatch
};

A_NODISCARD A_CONSTEXPR bool is_ok(status s) noexcept {
    return s == status::ok;
}
} // namespace a

