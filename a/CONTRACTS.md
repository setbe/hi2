# a:: Contracts

Short reference for `a/` contract discipline.

## Core Rules

- Use `a::contract::require(...)` for **API preconditions**.
- Use `a::contract::ensure(...)` for **postconditions** that must hold after operation.
- Use `a::contract::invariant(...)` for **internal state invariants**.
- Use `a::contract::assume_verified(...)` only when condition is already proven by prior checks.
- Use `a::contract::assume(...)` for checked assumption + optional optimizer hint.
- Use `a::contract::unreachable()` for impossible control-flow paths.

All contract failures end in `panic()` (non-returning hard fail).

## When To Use What

### `require(cond)`

Use when caller input must satisfy a strict boundary.

Examples:

- `ptr != nullptr`
- `count > 0`
- `align` is power-of-two

### `ensure(cond)`

Use when function output/state must satisfy an explicit postcondition.

Examples:

- resulting offset does not exceed arena size
- generated digest length is exact

### `invariant(cond)`

Use for object/module consistency that must always hold between operations.

Examples:

- `offset <= size`
- layout component index uniqueness assumptions

### `assume_verified(cond)`

Use only after the condition is already validated by logic/contracts.

Examples:

- after `require(is_pow2(align))`, internal fast-path may use `assume_verified(is_pow2(align))`

### `assume(cond)`

Checked assumption path:

- if `cond` is false: panic (no silent pass)
- if `cond` is true: may emit optimizer hint depending on `a::profile`

Use for hot internal paths where constraint is expected and failing it is a contract violation.

### `unreachable()`

Use only for truly impossible branches.
It is modeled as hard-fail contract, not as recovery path.

## Profile Notes (`a/profile.hpp`)

- One build has one global default profile (`profile::current_level`).
- Available presets: `strict`, `audit`, `lean`.
- `audit` keeps checks but disables optimizer assume hints.
- If a subsystem needs explicit level, use `*_as<profile::level::...>()`.

## Trusted Path Rules (`a/bounded.hpp`)

`a::bounded` and `a::pow2_bounded` maintain validated values.

- Public construction paths (`ctor`, `set`, `try_make`) enforce constraints.
- Internal trusted construction is private (`make_trusted`) and reserved for core composition (`pow2_bounded`, `static_bounded`).
- Trusted path must not be used as convenience bypass for user input.

## Practical Checklist

- API boundary check: `require`
- Internal consistency: `invariant`
- Final state check: `ensure`
- Proven hot-path hint: `assume_verified`
- Checked hint with same failure semantics: `assume`
- Impossible switch/default: `unreachable`
