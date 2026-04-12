#pragma once

#include "config.hpp"
#include "types.hpp"

namespace a {
namespace profile {
enum class level : u8 {
    strict = 0,
    audit,
    lean
};

struct settings {
    bool check_require;
    bool check_ensure;
    bool check_invariant;
    bool check_assume;
    bool emit_assume_hint;
};

template<level Level>
struct preset;

template<>
struct preset<level::strict> {
    static A_CONSTEXPR settings value() noexcept {
        return settings{true, true, true, true, true};
    }
};

template<>
struct preset<level::audit> {
    // Audit mode keeps full contract checking but suppresses optimizer assumptions.
    static A_CONSTEXPR settings value() noexcept {
        return settings{true, true, true, true, false};
    }
};

template<>
struct preset<level::lean> {
    // Lean mode keeps API/precondition checks; internal/post checks are trimmed.
    static A_CONSTEXPR settings value() noexcept {
        return settings{true, false, false, true, true};
    }
};

#if defined(A_PROFILE_STRICT)
#  define A_PROFILE_SELECTED_STRICT 1
#else
#  define A_PROFILE_SELECTED_STRICT 0
#endif

#if defined(A_PROFILE_AUDIT)
#  define A_PROFILE_SELECTED_AUDIT 1
#else
#  define A_PROFILE_SELECTED_AUDIT 0
#endif

#if defined(A_PROFILE_LEAN)
#  define A_PROFILE_SELECTED_LEAN 1
#else
#  define A_PROFILE_SELECTED_LEAN 0
#endif

static_assert((A_PROFILE_SELECTED_STRICT + A_PROFILE_SELECTED_AUDIT + A_PROFILE_SELECTED_LEAN) <= 1,
              "a::profile: define at most one of A_PROFILE_STRICT, A_PROFILE_AUDIT, A_PROFILE_LEAN");

#if A_PROFILE_SELECTED_LEAN
static A_CONSTEXPR_VAR level current_level = level::lean;
#elif A_PROFILE_SELECTED_AUDIT
static A_CONSTEXPR_VAR level current_level = level::audit;
#else
static A_CONSTEXPR_VAR level current_level = level::strict;
#endif

// Global default profile for the current build.
// Use a::contract::*_as<profile::level::...>() when a subsystem needs an explicit level.
A_NODISCARD A_CONSTEXPR settings current() noexcept {
    return preset<current_level>::value();
}

static_assert(preset<level::strict>::value().check_require,
              "a::profile: strict profile must keep require checks enabled");
static_assert(preset<level::audit>::value().check_require,
              "a::profile: audit profile must keep require checks enabled");
static_assert(preset<level::lean>::value().check_require,
              "a::profile: lean profile must keep require checks enabled");
static_assert(preset<level::strict>::value().check_assume,
              "a::profile: strict profile must keep assume checks enabled");
static_assert(preset<level::audit>::value().check_assume,
              "a::profile: audit profile must keep assume checks enabled");
static_assert(preset<level::lean>::value().check_assume,
              "a::profile: lean profile must keep assume checks enabled");

A_NODISCARD A_CONSTEXPR bool contracts_enabled() noexcept {
    const settings s = current();
    return s.check_require || s.check_ensure || s.check_invariant || s.check_assume ||
           s.emit_assume_hint;
}

#undef A_PROFILE_SELECTED_STRICT
#undef A_PROFILE_SELECTED_AUDIT
#undef A_PROFILE_SELECTED_LEAN
} // namespace profile
} // namespace a
