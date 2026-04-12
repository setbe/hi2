#pragma once

#include "../config.hpp"

namespace a {
namespace detail {
struct arch_generic {};
struct arch_x86_32 {};
struct arch_x86_64 {};
struct arch_arm32 {};
struct arch_arm64 {};

#if A_ARCH_X86_64
using arch_default = arch_x86_64;
#elif A_ARCH_X86_32
using arch_default = arch_x86_32;
#elif A_ARCH_ARM64
using arch_default = arch_arm64;
#elif A_ARCH_ARM32
using arch_default = arch_arm32;
#else
using arch_default = arch_generic;
#endif
} // namespace detail
} // namespace a

