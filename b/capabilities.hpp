#pragma once

#include "../a/config.hpp"

#ifndef B_HAS_THREADS
#  if defined(_WIN32) || defined(__linux__)
#    define B_HAS_THREADS 1
#  else
#    define B_HAS_THREADS 0
#  endif
#endif

#ifndef B_HAS_TIME
#  if defined(_WIN32) || defined(__linux__)
#    define B_HAS_TIME 1
#  else
#    define B_HAS_TIME 0
#  endif
#endif

#ifndef B_HAS_FILESYSTEM
#  if defined(_WIN32) || defined(__linux__)
#    define B_HAS_FILESYSTEM 1
#  else
#    define B_HAS_FILESYSTEM 0
#  endif
#endif

#ifndef B_HAS_SOCKETS
#  if defined(_WIN32) || defined(__linux__)
#    define B_HAS_SOCKETS 1
#  else
#    define B_HAS_SOCKETS 0
#  endif
#endif

#ifndef B_HAS_WINDOWING
#  if defined(_WIN32) || defined(__linux__)
#    define B_HAS_WINDOWING 1
#  else
#    define B_HAS_WINDOWING 0
#  endif
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
