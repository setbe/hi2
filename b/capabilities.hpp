#pragma once

#include "../a/config.hpp"

#if defined(_WIN32)
#  define B_OS_WINDOWS 1
#else
#  define B_OS_WINDOWS 0
#endif

#if defined(__linux__)
#  define B_OS_LINUX 1
#else
#  define B_OS_LINUX 0
#endif

#ifndef B_HAS_THREADS
#  if (B_OS_WINDOWS || B_OS_LINUX)
#    define B_HAS_THREADS 1
#  else
#    define B_HAS_THREADS 0
#  endif
#endif

#ifndef B_HAS_TIME
#  if (B_OS_WINDOWS || B_OS_LINUX)
#    define B_HAS_TIME 1
#  else
#    define B_HAS_TIME 0
#  endif
#endif

#ifndef B_HAS_FILESYSTEM
#  define B_HAS_FILESYSTEM 0
#endif

#ifndef B_HAS_SOCKETS
#  if (B_OS_WINDOWS || B_OS_LINUX)
#    define B_HAS_SOCKETS 1
#  else
#    define B_HAS_SOCKETS 0
#  endif
#endif

#ifndef B_HAS_WINDOWING
#  define B_HAS_WINDOWING 0
#endif

namespace b {
namespace cap {
static A_CONSTEXPR_VAR bool threads = (B_HAS_THREADS != 0);
static A_CONSTEXPR_VAR bool time = (B_HAS_TIME != 0);
static A_CONSTEXPR_VAR bool filesystem = (B_HAS_FILESYSTEM != 0);
static A_CONSTEXPR_VAR bool sockets = (B_HAS_SOCKETS != 0);
static A_CONSTEXPR_VAR bool windowing = (B_HAS_WINDOWING != 0);
} // namespace cap
} // namespace b
