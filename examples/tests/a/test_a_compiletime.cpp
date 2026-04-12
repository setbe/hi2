#include "a/a.hpp"
#include "a/bounded.hpp"
#include "a/contract.hpp"
#include "a/profile.hpp"

#include "a/crypto/chacha20.hpp"
#include "a/crypto/poly1305.hpp"
#include "a/crypto/aead_chacha20_poly1305.hpp"
#include "a/crypto/sha256.hpp"
#include "a/crypto/sha384.hpp"
#include "a/crypto/x25519.hpp"

struct font_spec {
    static constexpr a::memory_req requirement() noexcept { return {3, 1}; }
};

struct image_spec {
    static constexpr a::memory_req requirement() noexcept { return {8, 8}; }
};

struct atlas_spec {
    static constexpr a::memory_req requirement() noexcept { return {2, 2}; }
};

using packed_layout = a::layout<font_spec, image_spec, atlas_spec>;

static_assert(a::is_power_of_two(1), "pow2");
static_assert(a::is_power_of_two(8), "pow2");
static_assert(!a::is_power_of_two(6), "not pow2");

static_assert(a::align_up(3, 8) == 8, "align_up");

constexpr a::memory_req req_a{3, 1};
constexpr a::memory_req req_b{8, 8};
constexpr a::memory_req req_c{2, 2};

constexpr a::memory_req req_sum_ab = a::req_sum(req_a, req_b);
static_assert(req_sum_ab.bytes == 16, "req_sum bytes");
static_assert(req_sum_ab.align == 8, "req_sum align");

constexpr a::memory_req req_sum_abc = a::req_sum(req_sum_ab, req_c);
static_assert(req_sum_abc.bytes == 18, "req_sum chain bytes");
static_assert(req_sum_abc.align == 8, "req_sum chain align");

constexpr a::memory_req req_max_ab = a::req_max(req_a, req_b);
static_assert(req_max_ab.bytes == 8, "req_max bytes");
static_assert(req_max_ab.align == 8, "req_max align");

static_assert(a::sum_req<font_spec, image_spec, atlas_spec>::value.bytes == 18, "sum_req bytes");
static_assert(a::sum_req<font_spec, image_spec, atlas_spec>::value.align == 8, "sum_req align");
static_assert(a::max_req<font_spec, image_spec, atlas_spec>::value.bytes == 8, "max_req bytes");
static_assert(a::max_req<font_spec, image_spec, atlas_spec>::value.align == 8, "max_req align");

static_assert(packed_layout::offset_of<font_spec>() == 0, "layout offset font");
static_assert(packed_layout::offset_of<image_spec>() == 8, "layout offset image");
static_assert(packed_layout::offset_of<atlas_spec>() == 16, "layout offset atlas");
static_assert(packed_layout::component_count == 3, "layout component count");

using image_width = a::bounded<a::u32, 1, 8192>;
static_assert(image_width::in_range(1), "width min");
static_assert(image_width::in_range(8192), "width max");
static_assert(!image_width::in_range(0), "width low fail");
static_assert(!image_width::in_range(8193), "width high fail");

using align_pow2 = a::pow2_bounded<a::usize, 1, 4096>;
static_assert(align_pow2::in_range(1), "align min");
static_assert(align_pow2::in_range(16), "align pow2");
static_assert(!align_pow2::in_range(3), "align nonpow2");
static_assert(!align_pow2::in_range(8192), "align out of range");

using glyphs_static = a::static_bounded<a::u32, 256, 1, 4096>;
static_assert(glyphs_static::value == 256, "static bounded value");

static_assert(a::profile::preset<a::profile::level::strict>::value().check_require,
              "strict keeps require checks");
static_assert(a::profile::preset<a::profile::level::audit>::value().check_require,
              "audit keeps require checks");
static_assert(!a::profile::preset<a::profile::level::audit>::value().emit_assume_hint,
              "audit disables assume hints");

using contract_fn = void (*)(bool) noexcept;
contract_fn require_strict = &a::contract::require_as<a::profile::level::strict>;
contract_fn ensure_lean = &a::contract::ensure_as<a::profile::level::lean>;
contract_fn assume_verified_audit = &a::contract::assume_verified_as<a::profile::level::audit>;

int main() {
    (void)require_strict;
    (void)ensure_lean;
    (void)assume_verified_audit;

    const image_width width{512};
    const align_pow2 align{64};
    const auto glyphs = glyphs_static::make();

    return (width.get() == 512 && align.get() == 64 && glyphs.get() == 256) ? 0 : 1;
}
