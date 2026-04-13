# a:: Contracts

Short reference for `a/` contract discipline.

## Core Rules

- Use `a::contract::require(...)` for **API preconditions**.
- Use `a::contract::require_that<Tag>(...)` when you need an explicit proof token for assume-paths.
- Use `a::contract::ensure(...)` for **postconditions** that must hold after operation.
- Use `a::contract::invariant(...)` for **internal state invariants**.
- Use `a::contract::assume_verified(proven<Tag>&&)` only with proof tokens from trusted paths.
- Use `a::contract::unreachable()` for impossible control-flow paths.

`assume(...)` and `assume(bool)` are intentionally deleted from the public API.

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

### `require_that<Tag>(cond) -> proven<Tag>`

Use when a runtime check should produce an explicit proof token for later assume paths.

Examples:

- `auto p = require_that<alignment_is_pow2>(is_pow2(align));`
- `auto p = require_that<idx_in_range>(idx < count);`

### `assume_verified(proven<Tag>&&)`

Use only with proof tokens.
Tokens are move-only and consumed on first use.
`assume_verified` is a proof-consumption marker, not an expression prover.

### `prove_static<Tag, Cond>()`

Compile-time proof source:

- `prove_static<Tag, true>()` requires `Cond == true`

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
- API boundary check with reusable proof: `require_that<Tag>`
- Internal consistency: `invariant`
- Final state check: `ensure`
- Consume previously proven token: `assume_verified(proven<Tag>&&)`
- Impossible switch/default: `unreachable`
