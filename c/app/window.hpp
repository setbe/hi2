#pragma once

#include "../../a/types.hpp"
#include "../../a/view.hpp"
#include "../../b/capabilities.hpp"

namespace c {
namespace app {
enum class key : a::u16 {
    none = 0,
    escape,
    enter,
    space,
    left,
    right,
    up,
    down
};

struct window_desc {
    a::u32 width = 1280;
    a::u32 height = 720;
    a::char_view title = "c.window";
};

struct event {
    enum class type : a::u8 {
        none = 0,
        close,
        resize,
        key_down,
        key_up
    };

    type kind = type::none;
    key key_code = key::none;
    a::u32 width = 0;
    a::u32 height = 0;
};

struct window_handle {
    a::u64 opaque = 0;
};

#if B_HAS_WINDOWING
A_NODISCARD auto create(const window_desc& desc) noexcept -> window_handle;
auto destroy(window_handle h) noexcept -> void;
A_NODISCARD auto poll(window_handle h, event* out) noexcept -> bool;
#else
A_NODISCARD inline auto create(const window_desc&) noexcept -> window_handle = delete;
inline auto destroy(window_handle) noexcept -> void = delete;
A_NODISCARD inline auto poll(window_handle, event*) noexcept -> bool = delete;
#endif
} // namespace app
} // namespace c
