#pragma once

#include "config.hpp"
#include "memory_requirements.hpp"
#include "status.hpp"
#include "types.hpp"

namespace a {
struct alloc_result {
    void* ptr = nullptr;
    status code = status::ok;
};

struct arena_view {
    u8* data = nullptr;
    usize size = 0;
    usize offset = 0;

    A_NODISCARD A_CONSTEXPR bool empty() const noexcept { return size == 0; }
    A_NODISCARD A_CONSTEXPR bool valid_storage() const noexcept { return empty() || data != nullptr; }
    A_NODISCARD A_CONSTEXPR bool valid() const noexcept { return valid_storage(); }
    A_NODISCARD A_CONSTEXPR usize mark() const noexcept { return offset; }

    A_CONSTEXPR void rewind(usize m) noexcept {
        offset = (m <= size) ? m : size;
    }

    A_CONSTEXPR void reset() noexcept {
        offset = 0;
    }

    A_NODISCARD A_CONSTEXPR usize remaining() const noexcept {
        return (offset <= size) ? (size - offset) : 0;
    }

    A_NODISCARD A_CONSTEXPR static bool can_align(usize align) noexcept {
        return is_power_of_two(align);
    }

    A_NODISCARD alloc_result alloc(usize bytes, usize align = alignof(void*)) noexcept {
        if ((bytes == 0) || !can_align(align)) {
            return {nullptr, status::invalid_input};
        }
        if (!valid_storage()) {
            return {nullptr, status::invalid_input};
        }
        if (offset > size) {
            return {nullptr, status::mismatch};
        }

        const usize base = offset;
        const usize mask = align - 1;
        const usize aligned = (base + mask) & ~mask;
        if (aligned < base) {
            return {nullptr, status::overflow};
        }
        if (bytes > size - aligned) {
            return {nullptr, status::insufficient_memory};
        }

        u8* p = data + aligned;
        offset = aligned + bytes;
        return {p, status::ok};
    }

    template<typename T>
    A_NODISCARD T* alloc_object(status* out_code = nullptr) noexcept {
        const alloc_result r = alloc(sizeof(T), alignof(T));
        if (out_code != nullptr) {
            *out_code = r.code;
        }
        return static_cast<T*>(r.ptr);
    }

    template<typename T>
    A_NODISCARD T* alloc_array(usize count, status* out_code = nullptr) noexcept {
        if (count == 0) {
            if (out_code != nullptr) {
                *out_code = status::invalid_input;
            }
            return nullptr;
        }
        if (sizeof(T) != 0 && count > (static_cast<usize>(-1) / sizeof(T))) {
            if (out_code != nullptr) {
                *out_code = status::overflow;
            }
            return nullptr;
        }
        const alloc_result r = alloc(sizeof(T) * count, alignof(T));
        if (out_code != nullptr) {
            *out_code = r.code;
        }
        return static_cast<T*>(r.ptr);
    }
};

struct scope {
    arena_view* arena = nullptr;
    usize saved = 0;
    bool active = false;

    explicit scope(arena_view& a) noexcept
        : arena(&a), saved(a.mark()), active(true) {}

    scope(const scope&) = delete;
    scope& operator=(const scope&) = delete;

    scope(scope&& other) noexcept
        : arena(other.arena), saved(other.saved), active(other.active) {
        other.active = false;
    }

    scope& operator=(scope&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        if (active && arena != nullptr) {
            arena->rewind(saved);
        }
        arena = other.arena;
        saved = other.saved;
        active = other.active;
        other.active = false;
        return *this;
    }

    ~scope() noexcept {
        if (active && arena != nullptr) {
            arena->rewind(saved);
        }
    }

    void release() noexcept {
        active = false;
    }
};

} // namespace a
