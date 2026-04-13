#define CATCH_CONFIG_MAIN
#include "examples/tests/catch.hpp"

#include "a/arena.hpp"
#include "a/bit.hpp"
#include "a/bounded.hpp"
#include "a/contract.hpp"
#include "a/memory_requirements.hpp"
#include "a/two_pass.hpp"

namespace {
struct tracked_a {
    int value = 0;
    static int dtor_calls;

    tracked_a() noexcept = default;
    explicit tracked_a(int v) noexcept
        : value(v) {}

    ~tracked_a() noexcept { ++dtor_calls; }

    static constexpr a::memory_req requirement() noexcept {
        return {sizeof(tracked_a), alignof(tracked_a)};
    }
};

int tracked_a::dtor_calls = 0;

struct tracked_b {
    int value = 0;
    static int dtor_calls;

    tracked_b() noexcept = default;
    explicit tracked_b(int v) noexcept
        : value(v) {}

    ~tracked_b() noexcept { ++dtor_calls; }

    static constexpr a::memory_req requirement() noexcept {
        return {sizeof(tracked_b), alignof(tracked_b)};
    }
};

int tracked_b::dtor_calls = 0;
} // namespace

TEST_CASE("a::req_sum and a::req_max semantics", "[a][memory_req]") {
    const a::memory_req left{3, 1};
    const a::memory_req right{8, 8};

    const a::memory_req sum = a::req_sum(left, right);
    REQUIRE(sum.bytes == 16);
    REQUIRE(sum.align == 8);

    const a::memory_req maxv = a::req_max(left, right);
    REQUIRE(maxv.bytes == 8);
    REQUIRE(maxv.align == 8);
}

TEST_CASE("a::arena_view + a::scope rewinds on scope destruction", "[a][arena]") {
    a::u8 storage[64] = {};
    a::arena_view arena{storage, sizeof(storage), 0};

    REQUIRE(arena.offset == 0);
    {
        a::scope frame(arena);

        const a::alloc_result r1 = arena.alloc(8, 8);
        REQUIRE(r1.code == a::status::ok);
        REQUIRE(r1.ptr != nullptr);
        REQUIRE(arena.offset >= 8);

        const a::alloc_result r2 = arena.alloc(16, 8);
        REQUIRE(r2.code == a::status::ok);
        REQUIRE(r2.ptr != nullptr);
    }
    REQUIRE(arena.offset == 0);
}

TEST_CASE("a::typed_scope constructs, destroys and releases slots", "[a][typed_scope]") {
    using layout_t = a::layout<tracked_a, tracked_b>;

    tracked_a::dtor_calls = 0;
    tracked_b::dtor_calls = 0;

    a::u8 storage[256] = {};
    a::arena_view arena{storage, sizeof(storage), 0};

    {
        a::typed_scope<layout_t> scope(arena);
        REQUIRE(scope.init_status() == a::status::ok);
        REQUIRE(scope.active());

        auto* first = scope.construct<tracked_a>(11);
        REQUIRE(first != nullptr);
        REQUIRE(first->value == 11);
        REQUIRE(scope.is_constructed<tracked_a>());

        auto* second = scope.construct<tracked_b>(22);
        REQUIRE(second != nullptr);
        REQUIRE(second->value == 22);
        REQUIRE(scope.constructed_count() == 2);

        REQUIRE(scope.destroy<tracked_b>());
        REQUIRE_FALSE(scope.is_constructed<tracked_b>());
        REQUIRE(scope.constructed_count() == 1);

        REQUIRE(scope.release());
        REQUIRE_FALSE(scope.active());
        REQUIRE(scope.data() == nullptr);
        REQUIRE(scope.constructed_count() == 0);

        REQUIRE_FALSE(scope.release());
    }

    REQUIRE(tracked_a::dtor_calls == 1);
    REQUIRE(tracked_b::dtor_calls == 1);
}

TEST_CASE("a::typed_scope destructor destroys remaining constructed objects", "[a][typed_scope]") {
    using layout_t = a::layout<tracked_a, tracked_b>;

    tracked_a::dtor_calls = 0;
    tracked_b::dtor_calls = 0;

    a::u8 storage[256] = {};
    a::arena_view arena{storage, sizeof(storage), 0};

    {
        a::typed_scope<layout_t> scope(arena);
        REQUIRE(scope.construct<tracked_a>(7) != nullptr);
        REQUIRE(scope.construct<tracked_b>(9) != nullptr);
    }

    REQUIRE(tracked_a::dtor_calls == 1);
    REQUIRE(tracked_b::dtor_calls == 1);
    REQUIRE(arena.offset == 0);
}

TEST_CASE("a::bounded and a::pow2_bounded validate ranges", "[a][bounded]") {
    using channels = a::bounded<a::u8, 1, 4>;
    channels ch{2};
    REQUIRE(ch.get() == 2);

    channels parsed{};
    REQUIRE_FALSE(channels::try_make(0, parsed));
    REQUIRE(channels::try_make(4, parsed));
    REQUIRE(parsed.get() == 4);

    using align_t = a::pow2_bounded<a::usize, 1, 64>;
    align_t align{8};
    REQUIRE(align.get() == 8);

    align_t parsed_align{};
    REQUIRE_FALSE(align_t::try_make(6, parsed_align));
    REQUIRE(align_t::try_make(32, parsed_align));
    REQUIRE(parsed_align.get() == 32);
}

TEST_CASE("a::bitset tracks constructed flags", "[a][bitset]") {
    a::bitset<10> flags{};
    REQUIRE_FALSE(flags.any());
    REQUIRE(flags.count() == 0);

    REQUIRE(flags.set(1));
    REQUIRE(flags.set(7));
    REQUIRE(flags.any());
    REQUIRE(flags.test(1));
    REQUIRE(flags.test(7));
    REQUIRE(flags.count() == 2);

    REQUIRE(flags.reset(1));
    REQUIRE_FALSE(flags.test(1));
    REQUIRE(flags.count() == 1);

    flags.clear();
    REQUIRE_FALSE(flags.any());
    REQUIRE(flags.count() == 0);
}

namespace {
struct alignment_is_pow2_tag {};
} // namespace

TEST_CASE("a::contract proof tokens drive assume paths", "[a][contract]") {
    auto proof_verified = a::contract::require_that<alignment_is_pow2_tag>(true);
    a::contract::assume_verified(a::move(proof_verified));

    auto static_proof = a::contract::prove_static<alignment_is_pow2_tag, true>();
    a::contract::assume_verified(a::move(static_proof));

    auto audit_proof =
        a::contract::require_that_as<alignment_is_pow2_tag, a::profile::level::audit>(true);
    a::contract::assume_verified_as<a::profile::level::audit>(a::move(audit_proof));
}
