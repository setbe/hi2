#pragma once

#include "config.hpp"
#include "types.hpp"

namespace a {
// Generic helper: clamp to decimal digit range [0..9] and truncate fractional part.
A_NODISCARD static inline u32 digit_floor_clamped_0_9(double x) noexcept {
    if (!(x == x) || x <= 0.0) {
        return 0;
    }
    if (x >= 9.0) {
        return 9;
    }
    return static_cast<u32>(x);
}

// Legacy-compatible helper from old io.hpp contract:
// expected precondition: 0 <= x < 10.
// For out-of-contract inputs we fall back to clamped behavior.
A_NODISCARD static inline u32 digit_to_u32_from_double_pos(double x) noexcept {
    if (x >= 0.0 && x < 10.0) {
        return static_cast<u32>(x); // trunc toward zero for positive values
    }
    return digit_floor_clamped_0_9(x);
}

// Decimal digit only helper: input is expected in [0..9].
A_NODISCARD static inline double digit_to_double(u32 d) noexcept {
    switch (d) {
    case 0: return 0.0;
    case 1: return 1.0;
    case 2: return 2.0;
    case 3: return 3.0;
    case 4: return 4.0;
    case 5: return 5.0;
    case 6: return 6.0;
    case 7: return 7.0;
    case 8: return 8.0;
    default: return 9.0;
    }
}
} // namespace a
