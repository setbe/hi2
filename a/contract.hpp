#pragma once

#include "config.hpp"
#include "profile.hpp"

namespace a {
namespace contract {
namespace detail {
A_FORCE_INLINE void emit_assume_hint(bool cond) noexcept {
#if A_COMPILER_MSVC
    __assume(cond);
#elif A_COMPILER_CLANG
    __builtin_assume(cond);
#elif A_COMPILER_GCC
    if (!cond) {
        __builtin_unreachable();
    }
#else
    (void)cond;
#endif
}
} // namespace detail

[[noreturn]] A_FORCE_INLINE void panic(const char* msg) noexcept {
    (void)msg;

    // Optional hook for logging/telemetry only.
    // Recovery is not supported: panic() never returns.
#if defined(A_CONTRACT_PANIC)
    A_CONTRACT_PANIC(msg);
#endif

    // Optional trap hook (for hosted/debug integrations on toolchains without builtin_trap).
#if defined(A_CONTRACT_TRAP)
    A_CONTRACT_TRAP();
#endif

#if A_COMPILER_CLANG || A_COMPILER_GCC
    __builtin_trap();
#endif

    for (;;) {}
}

template<profile::level Level>
A_FORCE_INLINE void require_as(bool cond) noexcept {
    const profile::settings s = profile::preset<Level>::value();
    if (s.check_require && A_UNLIKELY(!cond)) {
        panic("require");
    }
}

template<profile::level Level>
A_FORCE_INLINE void ensure_as(bool cond) noexcept {
    const profile::settings s = profile::preset<Level>::value();
    if (s.check_ensure && A_UNLIKELY(!cond)) {
        panic("ensure");
    }
}

template<profile::level Level>
A_FORCE_INLINE void invariant_as(bool cond) noexcept {
    const profile::settings s = profile::preset<Level>::value();
    if (s.check_invariant && A_UNLIKELY(!cond)) {
        panic("invariant");
    }
}

template<profile::level Level>
A_FORCE_INLINE void assume_as(bool cond) noexcept;

// `assume_verified` is for conditions already proven by prior contract checks.
// It never introduces UB: false still goes through panic().
template<profile::level Level>
A_FORCE_INLINE void assume_verified_as(bool cond) noexcept {
    assume_as<Level>(cond);
}

template<profile::level Level>
A_FORCE_INLINE void assume_as(bool cond) noexcept {
    const profile::settings s = profile::preset<Level>::value();
    static_assert(profile::preset<Level>::value().check_assume,
                  "a::contract::assume_as<Level>: profile must keep assume checks enabled");
    if (A_UNLIKELY(!cond)) {
        panic("assume");
    }
    if (s.emit_assume_hint) {
        detail::emit_assume_hint(cond);
    }
}

[[noreturn]] A_FORCE_INLINE void unreachable() noexcept {
    // Hard-fail contract: no recovery, no continuation.
    panic("unreachable");
}

A_FORCE_INLINE void require(bool cond) noexcept {
    require_as<profile::current_level>(cond);
}

A_FORCE_INLINE void ensure(bool cond) noexcept {
    ensure_as<profile::current_level>(cond);
}

A_FORCE_INLINE void invariant(bool cond) noexcept {
    invariant_as<profile::current_level>(cond);
}

A_FORCE_INLINE void assume_verified(bool cond) noexcept {
    assume_verified_as<profile::current_level>(cond);
}

A_FORCE_INLINE void assume(bool cond) noexcept {
    // Checked assume: fails hard on false, optional optimizer hint on true.
    assume_as<profile::current_level>(cond);
}
} // namespace contract
} // namespace a
