#pragma once

#include "arena.hpp"
#include "bit.hpp"
#include "meta.hpp"
#include "memory_requirements.hpp"
#include "status.hpp"
#include "types.hpp"
#include "utility.hpp"
#include <new>

namespace a {
// Default component contract: Component::requirement().
// For more advanced specs, specialize component_req<T>.
template<typename Component>
struct component_req {
    static constexpr memory_req value = Component::requirement();

    A_NODISCARD static constexpr memory_req requirement() noexcept {
        return value;
    }
};

namespace detail {
template<typename T, typename... Components>
struct count_type;

template<typename T>
struct count_type<T> {
    static A_CONSTEXPR_VAR usize value = 0;
};

template<typename T, typename Head, typename... Tail>
struct count_type<T, Head, Tail...> {
    static A_CONSTEXPR_VAR usize value =
        (is_same<T, Head>::value ? 1u : 0u) + count_type<T, Tail...>::value;
};

template<typename... Components>
struct all_unique;

template<>
struct all_unique<> {
    static A_CONSTEXPR_VAR bool value = true;
};

template<typename Head, typename... Tail>
struct all_unique<Head, Tail...> {
    static A_CONSTEXPR_VAR bool value =
        (count_type<Head, Tail...>::value == 0) && all_unique<Tail...>::value;
};

template<typename... Components>
struct all_nothrow_destructible;

template<>
struct all_nothrow_destructible<> {
    static A_CONSTEXPR_VAR bool value = true;
};

template<typename Head, typename... Tail>
struct all_nothrow_destructible<Head, Tail...> {
    static A_CONSTEXPR_VAR bool value =
        is_nothrow_destructible_v<Head> && all_nothrow_destructible<Tail...>::value;
};

template<typename T, usize Index, typename... Components>
struct index_type;

template<typename T, usize Index>
struct index_type<T, Index> {
    static A_CONSTEXPR_VAR usize value = req_overflow_bytes;
};

template<typename T, usize Index, typename Head, typename... Tail>
struct index_type<T, Index, Head, Tail...> {
    static A_CONSTEXPR_VAR usize value =
        is_same<T, Head>::value ? Index : index_type<T, Index + 1, Tail...>::value;
};
} // namespace detail

template<typename... Components>
struct max_req;

template<typename First>
struct max_req<First> {
    static constexpr memory_req value = component_req<First>::value;

    A_NODISCARD static constexpr memory_req requirement() noexcept {
        return value;
    }
};

template<typename First, typename Second, typename... Rest>
struct max_req<First, Second, Rest...> {
    static constexpr memory_req value =
        req_max(component_req<First>::value, max_req<Second, Rest...>::value);

    A_NODISCARD static constexpr memory_req requirement() noexcept {
        return value;
    }
};

template<typename... Components>
struct sum_req;

template<typename First>
struct sum_req<First> {
    static constexpr memory_req value = component_req<First>::value;

    A_NODISCARD static constexpr memory_req requirement() noexcept {
        return value;
    }
};

template<typename First, typename Second, typename... Rest>
struct sum_req<First, Second, Rest...> {
private:
    template<typename Next>
    A_NODISCARD static constexpr memory_req fold(memory_req acc) noexcept {
        return req_sum(acc, component_req<Next>::value);
    }

    template<typename Next, typename Next2, typename... Tail>
    A_NODISCARD static constexpr memory_req fold(memory_req acc) noexcept {
        return fold<Next2, Tail...>(req_sum(acc, component_req<Next>::value));
    }

public:
    static constexpr memory_req value = fold<Second, Rest...>(component_req<First>::value);

    A_NODISCARD static constexpr memory_req requirement() noexcept {
        return value;
    }
};

namespace detail {
template<typename Target, usize Offset, typename... Components>
struct layout_offset;

template<typename Target, usize Offset>
struct layout_offset<Target, Offset> {
    A_NODISCARD static constexpr usize value() noexcept {
        return req_overflow_bytes;
    }
};

template<typename Target, usize Offset, typename Head, typename... Tail>
struct layout_offset<Target, Offset, Head, Tail...> {
    A_NODISCARD static constexpr memory_req head_req() noexcept {
        return component_req<Head>::value;
    }

    A_NODISCARD static constexpr usize head_offset() noexcept {
        return align_up(Offset, head_req().align);
    }

    A_NODISCARD static constexpr usize next_offset() noexcept {
        return req_sum(memory_req{Offset, 1}, head_req()).bytes;
    }

    A_NODISCARD static constexpr usize value() noexcept {
        return is_same<Target, Head>::value ? head_offset()
                                            : layout_offset<Target, next_offset(), Tail...>::value();
    }
};
} // namespace detail

template<typename... Components>
struct layout {
    static_assert(sizeof...(Components) > 0, "layout: at least one component is required");
    static_assert(detail::all_unique<Components...>::value,
                  "layout: duplicate component type is not allowed");

    static A_CONSTEXPR_VAR usize component_count = sizeof...(Components);
    static A_CONSTEXPR_VAR bool all_components_nothrow_destructible =
        detail::all_nothrow_destructible<Components...>::value;
    static constexpr memory_req value = sum_req<Components...>::value;
    static_assert(is_valid(value), "layout: invalid requirement (align/overflow)");
    static_assert(!is_overflow(value), "layout: requirement overflow");

    A_NODISCARD static constexpr memory_req requirement() noexcept {
        return value;
    }

    template<typename T>
    A_NODISCARD static constexpr bool has_component() noexcept {
        return detail::count_type<T, Components...>::value == 1;
    }

    template<typename T>
    A_NODISCARD static constexpr usize index_of() noexcept {
        static_assert(has_component<T>(),
                      "layout::index_of<T>: T must appear exactly once in Components...");
        constexpr usize idx = detail::index_type<T, 0, Components...>::value;
        static_assert(idx != req_overflow_bytes, "layout::index_of<T>: invalid index");
        return idx;
    }

    template<typename T>
    A_NODISCARD static constexpr usize offset_of() noexcept {
        static_assert(has_component<T>(),
                      "layout::offset_of<T>: T must appear exactly once in Components...");
        constexpr usize off = detail::layout_offset<T, 0, Components...>::value();
        static_assert(off != req_overflow_bytes, "layout::offset_of<T>: invalid offset");
        return off;
    }

    template<typename T>
    A_NODISCARD static T* ptr(void* base) noexcept {
        constexpr usize off = offset_of<T>();
        static_assert(off != req_overflow_bytes, "layout::ptr<T>: invalid offset");
        // Raw slot pointer only. Object lifetime is managed by typed_scope::construct/destroy.
        return a::launder_ptr(reinterpret_cast<T*>(static_cast<a::u8*>(base) + off));
    }

    template<typename T>
    A_NODISCARD static const T* ptr(const void* base) noexcept {
        constexpr usize off = offset_of<T>();
        static_assert(off != req_overflow_bytes, "layout::ptr<T>: invalid offset");
        return a::launder_ptr(reinterpret_cast<const T*>(static_cast<const a::u8*>(base) + off));
    }

private:
    template<usize Index>
    static void destroy_constructed_impl(void*, bitset<component_count>&) noexcept {}

    template<usize Index, typename Head, typename... Tail>
    static void destroy_constructed_impl(void* base, bitset<component_count>& flags) noexcept {
        destroy_constructed_impl<Index + 1, Tail...>(base, flags);
        if (flags.test(Index)) {
            ptr<Head>(base)->~Head();
            flags.reset(Index);
        }
    }

public:
    static void destroy_constructed(void* base, bitset<component_count>& flags) noexcept {
        if (base == nullptr) {
            return;
        }
        destroy_constructed_impl<0, Components...>(base, flags);
    }
};

template<typename... Components>
struct scope_spec {
    using layout_type = layout<Components...>;
    static constexpr memory_req value = layout_type::value;
    static_assert(is_valid(value), "scope_spec: invalid requirement");
    static_assert(!is_overflow(value), "scope_spec: requirement overflow");

    A_NODISCARD static constexpr memory_req requirement() noexcept {
        return value;
    }
};

template<typename Layout>
struct typed_scope {
    static_assert(Layout::component_count > 0, "typed_scope: layout must contain at least one component");
    static_assert(Layout::all_components_nothrow_destructible,
                  "typed_scope: all layout components must be nothrow-destructible");
    static_assert(is_valid(Layout::requirement()), "typed_scope: invalid layout requirement");
    static_assert(!is_overflow(Layout::requirement()), "typed_scope: layout requirement overflow");

    a::scope frame;
    void* base = nullptr;
    // Initialization status only (from ctor allocation/validation path).
    // Usability is defined by is_alive(), not by init_status().
    status init_code = status::ok;
    bool alive = false;
    bitset<Layout::component_count> constructed{};

    explicit typed_scope(arena_view& arena) noexcept
        : frame(arena) {
        const memory_req req = Layout::requirement();
        if (!is_valid(req) || is_overflow(req)) {
            init_code = status::overflow;
            alive = false;
            return;
        }
        const alloc_result r = arena.alloc(req.bytes, req.align);
        base = r.ptr;
        init_code = r.code;
        alive = (base != nullptr) && is_ok(init_code);
    }

    typed_scope(const typed_scope&) = delete;
    typed_scope& operator=(const typed_scope&) = delete;

    typed_scope(typed_scope&& other) noexcept
        : frame(a::move(other.frame)), base(other.base), init_code(other.init_code), alive(other.alive) {
        constructed = other.constructed;
        other.make_inert();
    }

    typed_scope& operator=(typed_scope&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        // Destructive move: current constructed objects are destroyed before takeover.
        destroy_all();

        frame = a::move(other.frame);
        base = other.base;
        init_code = other.init_code;
        alive = other.alive;
        constructed = other.constructed;
        other.make_inert();
        return *this;
    }

    ~typed_scope() noexcept {
        destroy_all();
    }

    A_NODISCARD static constexpr memory_req requirement() noexcept {
        return Layout::requirement();
    }

    // Usability check. `init_status()==ok` alone is not sufficient; inert scopes return false.
    A_NODISCARD A_CONSTEXPR explicit operator bool() const noexcept {
        return is_alive();
    }

    A_NODISCARD status init_status() const noexcept { return init_code; }
    A_NODISCARD bool active() const noexcept { return is_alive(); }

    template<typename T>
    A_NODISCARD bool is_constructed() const noexcept {
        static_assert(Layout::template has_component<T>(),
                      "typed_scope::is_constructed<T>: T must belong to Layout");
        if (!is_alive()) {
            return false;
        }
        const usize idx = Layout::template index_of<T>();
        return constructed.test(idx);
    }

    A_NODISCARD usize constructed_count() const noexcept {
        return constructed.count();
    }

    // Raw storage slot (no lifetime guarantee).
    template<typename T>
    A_NODISCARD T* slot() noexcept {
        static_assert(Layout::template has_component<T>(),
                      "typed_scope::slot<T>: T must belong to Layout");
        if (!is_alive()) {
            return nullptr;
        }
        return Layout::template ptr<T>(base);
    }

    template<typename T>
    A_NODISCARD const T* slot() const noexcept {
        static_assert(Layout::template has_component<T>(),
                      "typed_scope::slot<T>: T must belong to Layout");
        if (!is_alive()) {
            return nullptr;
        }
        return Layout::template ptr<T>(base);
    }

    // Constructed object view. Returns nullptr when object lifetime was not started.
    template<typename T>
    A_NODISCARD T* get() noexcept {
        static_assert(Layout::template has_component<T>(),
                      "typed_scope::get<T>: T must belong to Layout");
        const usize idx = Layout::template index_of<T>();
        return (is_alive() && constructed.test(idx)) ? Layout::template ptr<T>(base) : nullptr;
    }

    template<typename T>
    A_NODISCARD const T* get() const noexcept {
        static_assert(Layout::template has_component<T>(),
                      "typed_scope::get<T>: T must belong to Layout");
        const usize idx = Layout::template index_of<T>();
        return (is_alive() && constructed.test(idx)) ? Layout::template ptr<T>(base) : nullptr;
    }

    template<typename T, typename... Args>
    A_NODISCARD T* construct(Args&&... args) noexcept {
        static_assert(is_nothrow_destructible_v<T>,
                      "typed_scope::construct<T>: T must be nothrow-destructible");
        static_assert(is_nothrow_constructible_v<T, Args...>,
                      "typed_scope::construct<T>: T must be nothrow-constructible for Args...");
        static_assert(Layout::template has_component<T>(),
                      "typed_scope::construct<T>: T must belong to Layout");

        T* p = slot<T>();
        if (p == nullptr) {
            return nullptr;
        }
        const usize idx = Layout::template index_of<T>();
        if (constructed.test(idx)) {
            return nullptr;
        }
        T* obj = ::new (static_cast<void*>(p)) T(a::forward<Args>(args)...);
        constructed.set(idx);
        return obj;
    }

    template<typename T>
    A_NODISCARD bool destroy() noexcept {
        static_assert(is_nothrow_destructible_v<T>,
                      "typed_scope::destroy<T>: T must be nothrow-destructible");
        static_assert(Layout::template has_component<T>(),
                      "typed_scope::destroy<T>: T must belong to Layout");

        T* p = slot<T>();
        if (p == nullptr) {
            return false;
        }
        const usize idx = Layout::template index_of<T>();
        if (!constructed.test(idx)) {
            return false;
        }
        p->~T();
        constructed.reset(idx);
        return true;
    }

    // Cleanup-only: destroys constructed objects, but keeps storage open.
    // Use `release()` to close the scope and make it inert.
    void destroy_all() noexcept {
        // Allowed in error state: this is a cleanup path, not a usage path.
        if (base == nullptr) {
            return;
        }
        Layout::destroy_constructed(base, constructed);
    }

    A_NODISCARD bool release() noexcept {
        if (!is_alive()) {
            return false;
        }
        if (constructed.any()) {
            destroy_all();
        }
        frame.release();
        make_inert();
        return true;
    }

    // Unsafe escape hatch: bypasses slot/get lifetime discipline.
    A_NODISCARD void* data() noexcept { return base; }
    A_NODISCARD const void* data() const noexcept { return base; }

private:
    A_NODISCARD bool is_alive() const noexcept {
        return alive && (base != nullptr) && is_ok(init_code);
    }

    void make_inert() noexcept {
        // Invariant: inert => no base storage and no constructed objects.
        frame.release();
        base = nullptr;
        alive = false;
        constructed.clear();
    }
};

namespace two_pass {
template<typename Plan>
struct plan_result {
    status code = status::ok;
    Plan plan{};
    memory_req memory{};

    A_NODISCARD A_CONSTEXPR explicit operator bool() const noexcept {
        return code == status::ok;
    }
};

template<typename Output>
struct build_result {
    status code = status::ok;
    Output value{};

    A_NODISCARD A_CONSTEXPR explicit operator bool() const noexcept {
        return code == status::ok;
    }
};

template<>
struct build_result<void> {
    status code = status::ok;

    A_NODISCARD A_CONSTEXPR explicit operator bool() const noexcept {
        return code == status::ok;
    }
};

template<typename Plan, typename Input>
struct traits;

template<usize Bytes, usize Align = 1>
struct static_req {
    static_assert(Bytes > 0, "static_req: bytes must be > 0");
    static_assert(is_power_of_two(Align), "static_req: align must be power-of-two");
    static constexpr memory_req value = memory_req{Bytes, Align};

    A_NODISCARD static constexpr memory_req requirement() noexcept {
        return value;
    }
};

template<typename Component> struct component_req : a::component_req<Component> {};
template<typename... Components> struct max_req : a::max_req<Components...> {};
template<typename... Components> struct sum_req : a::sum_req<Components...> {};
template<typename... Components> struct layout : a::layout<Components...> {};
template<typename... Components>
struct scope : a::typed_scope<a::layout<Components...>> {
    using base = a::typed_scope<a::layout<Components...>>;
    explicit scope(arena_view& arena) noexcept : base(arena) {}
};
} // namespace two_pass
} // namespace a
