#pragma once

#include "config.hpp"

namespace a {
using i8 = signed char;
using u8 = unsigned char;

using i16 = short;
using u16 = unsigned short;

using i32 = int;
using u32 = unsigned int;

using i64 = long long;
using u64 = unsigned long long;

using isize = decltype(static_cast<char*>(nullptr) - static_cast<char*>(nullptr));
using usize = decltype(sizeof(0));

struct u128 {
    u64 lo;
    u64 hi;
};

static A_CONSTEXPR_VAR usize npos = static_cast<usize>(-1);
} // namespace a

