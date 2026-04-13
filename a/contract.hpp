#pragma once

#include "config.hpp"
#include "profile.hpp"
#include "utility.hpp"

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

template<typename Tag>
struct proven final {
private:
    struct token {};
    static A_CONSTEXPR_VAR u8 state_empty = 0;
    static A_CONSTEXPR_VAR u8 state_live = 1;
    static A_CONSTEXPR_VAR u8 state_consumed = 2;

    // Token invariant:
    // - newly created token: live
    // - moved-from token: empty
    // - consumed by assume_verified*: consumed
    // assume_verified* accepts only live tokens.
    u8 state = state_live;

    A_CONSTEXPR explicit proven(token) noexcept
        : state(state_live) {}

    A_NODISCARD A_CONSTEXPR bool live() const noexcept {
        return state == state_live;
    }

    A_FORCE_INLINE void consume() noexcept {
        state = state_consumed;
    }

    template<typename T, profile::level Level>
    friend A_NODISCARD A_FORCE_INLINE proven<T> require_that_as(bool cond) noexcept;

    template<typename T, bool Cond>
    friend A_NODISCARD A_CONSTEXPR proven<T> prove_static() noexcept;

    template<profile::level Level, typename T>
    friend A_FORCE_INLINE void assume_verified_as(proven<T>&& p) noexcept;

public:
    proven(const proven&) = delete;
    proven& operator=(const proven&) = delete;

    A_FORCE_INLINE proven(proven&& other) noexcept
        : state(other.state) {
        other.state = state_empty;
    }

    A_FORCE_INLINE proven& operator=(proven&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        state = other.state;
        other.state = state_empty;
        return *this;
    }
};

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

template<typename Tag, profile::level Level>
A_NODISCARD A_FORCE_INLINE proven<Tag> require_that_as(bool cond) noexcept {
    static_assert(profile::preset<Level>::value().check_require,
                  "a::contract::require_that_as<Level>: profile must keep require checks enabled");
    const profile::settings s = profile::preset<Level>::value();
    require_as<Level>(cond);
    // The hint is emitted exactly at the checked proof point.
    // We do not accept a second raw boolean in assume_verified(), to avoid
    // accidental "token for X, condition for Y" mismatches.
    if (s.emit_assume_hint) {
        detail::emit_assume_hint(cond);
    }
    return proven<Tag>(typename proven<Tag>::token{});
}

template<typename Tag, bool Cond>
A_NODISCARD A_CONSTEXPR proven<Tag> prove_static() noexcept {
    static_assert(Cond, "a::contract::prove_static<Tag, Cond>: Cond must be true");
    return proven<Tag>(typename proven<Tag>::token{});
}

template<profile::level Level, typename Tag>
A_FORCE_INLINE void assume_verified_as(proven<Tag>&& p) noexcept {
    static_assert(profile::preset<Level>::value().check_assume,
                  "a::contract::assume_verified_as<Level>: profile must keep assume checks enabled");
    if (A_UNLIKELY(!p.live())) {
        panic("assume_token");
    }
    p.consume();
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

template<typename Tag>
A_NODISCARD A_FORCE_INLINE proven<Tag> require_that(bool cond) noexcept {
    return require_that_as<Tag, profile::current_level>(cond);
}

template<typename Tag>
A_FORCE_INLINE void assume_verified(proven<Tag>&& p) noexcept {
    assume_verified_as<profile::current_level, Tag>(a::move(p));
}

template<typename Tag>
void assume(proven<Tag>&) = delete;
template<typename Tag>
void assume_verified(proven<Tag>&) = delete;
template<typename Tag>
void assume(proven<Tag>&&) = delete;

void assume(bool) = delete;
void assume_verified(bool) = delete;
} // namespace contract
} // namespace a
