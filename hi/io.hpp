#pragma once

#if !defined(_DEBUG) && !defined(NDEBUG)
#   define _DEBUG
#endif

#if defined(_CONSOLE) || defined(CONSOLE) && !defined(IO_TERMINAL)
#   define IO_TERMINAL
#endif

#ifdef None // Macro may come from XLib, we use `None` none of this in codebase
#   undef None
#endif

#if defined(_M_IX86) || defined(__i386__)
#   define IO_ARCH_X86_32 1
#endif

#if defined(_M_X64) || defined(__x86_64__)
#   define IO_ARCH_X86_64 1
#endif

#if IO_ARCH_X86_32 || IO_ARCH_X86_64
#   define IO_ARCH_X86 1
#endif

// ------------------------------ IO_API --------------------------------------
#ifndef IO_API
#  if defined(_WIN32) && defined(IO_BUILD_DLL)
#    define IO_API __declspec(dllexport)
#  elif defined(_WIN32) && defined(IO_USE_DLL)
#    define IO_API __declspec(dllimport)
#  else
#    define IO_API
#  endif
#endif

#ifndef IO_CDECL
#   if defined(_MSC_VER)
#      define IO_CDECL __cdecl
#   else
#      define IO_CDECL
#   endif
#endif

// ============================================================================
// Implementation (include OS headers ONLY here)
// Define IO_IMPLEMENTATION in exactly ONE translation unit before including.
// ============================================================================
#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32)
#       ifndef WIN32_LEAN_AND_MEAN
#           define WIN32_LEAN_AND_MEAN
#       endif
#       ifndef NOMINMAX
#           define NOMINMAX
#       endif
#       include <Windows.h>
#       include <bcrypt.h>
#   elif defined(__linux__)
#       include <pthread.h>
#       include <unistd.h>
#       include <sched.h>
#   else
#       error "OS isn't specified"
#   endif

#if !defined(IO_HAS_STD) && defined(_MSC_VER)
extern "C" {
    int _fltused = 0;

    void* IO_CDECL memset(void* dst, int c, size_t n) {
        unsigned char* p = (unsigned char*)dst;
        while (n--) *p++ = (unsigned char)c;
        return dst;
    }

    void* IO_CDECL memcpy(void* dst, const void* src, size_t n) {
        auto* d = (unsigned char*)dst;
        auto* s = (const unsigned char*)src;
        while (n--) *d++ = *s++;
        return dst;
    }

    // x86 decorated name used by MSVC sometimes
    void* IO_CDECL _memcpy(void* dst, const void* src, size_t n) {
        return memcpy(dst, src, n);
    }

    // required by compiler / CRT references (x86 decoration: _atexit)
    static int IO_CDECL _atexit(void(IO_CDECL* func)(void)) {
        (void)func; return 0; // Ignore register, or impl simple stack here.
    }
    // also provide plain atexit and onexit variants just in case
    static int IO_CDECL atexit(   void(IO_CDECL* func)(void)) { return _atexit(func); }
    static inline int IO_CDECL _purecall(void) {
#ifdef _MSC_VER
        __debugbreak();
#endif
        for (;;) {}
    }

#   if defined(_MSC_VER) && defined(IO_ARCH_X86_32)
        __declspec(naked) unsigned __int64 IO_CDECL _aullshr(void) {
            __asm {
                // in:  EDX:EAX = value, CL = shift
                // out: EDX:EAX = value >> CL
                and cl, 63
                cmp     cl, 32
                jb      short lt32
                // shift >= 32
                mov     eax, edx
                xor edx, edx
                and cl, 31
                shr     eax, cl
                ret
                lt32 :
                shrd    eax, edx, cl
                    shr     edx, cl
                    ret
            }
        }
        __declspec(naked) unsigned __int64 IO_CDECL _allshl(void) {
            __asm {
                // in:  EDX:EAX = value, CL = shift
                // out: EDX:EAX = value << CL
                and cl, 63
                cmp     cl, 32
                jb      short lt32
                // shift >= 32
                mov     edx, eax
                xor eax, eax
                and cl, 31
                shl     edx, cl
                ret
                lt32 :
                shld    edx, eax, cl
                    shl     eax, cl
                    ret
            }
        }

#       if defined(NDEBUG) && defined(IO_TERMINAL)
            int main();
            __declspec(noreturn) void IO_CDECL mainCRTStartup() {
                int rc = main();
                ExitProcess((UINT)rc);
            }
#       endif // NDEBUG
#   endif // x86
} // extern "C"

static void(IO_CDECL* _onexit(void(IO_CDECL* func)(void)))(void) {
    // minimal stub: return the pointer (but do not register)
    (void)func; return nullptr;
}
#endif // IO_HAS_STD
#endif // IO_IMPLEMENTATION

#pragma region macros

#ifndef IO_CACHELINE
#   define IO_CACHELINE 64
#endif

#ifndef IO_TERMINAL_BUFFER_SIZE
#   define IO_TERMINAL_BUFFER_SIZE 512
#endif

#if defined(_MSC_VER)
#   define IO_ALIGN(N) __declspec(align(N))
#else
#   define IO_ALIGN(N) __attribute__((aligned(N)))
#endif


// `main` was replaced with `WinMain` due to `_CONSOLE` macro absence
#if defined(_MSC_VER) && !defined(IO_TERMINAL)
#   define main() __stdcall WinMain( HINSTANCE hInstance,                     \
                                     HINSTANCE hPrevInstance,                 \
                                     LPSTR     lpCmdLine,                     \
                                     int       nCmdShow)
#endif

#define IO_DEFINE_GLOBAL_ARRAY_AND_COUNT(NAME, TYPE, COUNT)                   \
namespace global {                                                            \
    static TYPE NAME[COUNT]{};                                                \
    static ::io::usize NAME##_count{ 0 };                                     \
}

#pragma region io_keywords

// -------------------- C++17 Version Detection -------------------------------
#ifdef _MSC_VER // MSVC
#   if defined(_MSVC_LANG)
#       define IO_CPP_VERSION _MSVC_LANG
#   else
#       define IO_CPP_VERSION __cplusplus
#   endif
#elif defined(__GNUC__) || defined(__clang__) // GNU || CLang
#   if __cplusplus >= 201703L
#       define IO_CPP_VERSION __cplusplus
#   endif
#endif // C++17 feature detection

// --------------------- keyword `IO_RESTRICT` --------------------------------

#if IO_CPP_VERSION < 202302L // `restrict` was officially added in C++23
#  ifndef IO_RESTRICT
#    ifdef __GNUC__         // GNU
#      define IO_RESTRICT __restrict__
#    elif defined(_MSC_VER) // MSVC
#      define IO_RESTRICT __restrict
#    else                   // Fallback
#      define IO_RESTRICT
#    endif
#  endif // IO_RESTRICT
#endif // IO_CPP_VERSION < 202302L


#if IO_CPP_VERSION >= 201703L // Define `IO_CXX_17` macro if C++17 or above used
#    define IO_CXX_17
#endif

// -------------------------- IO_FALLTHROUGH ----------------------------------
#if defined(IO_CXX_17)
#   define IO_FALLTHROUGH [[fallthrough]]
#elif defined(__GNUC__) && __GNUC__ >= 7
#   define IO_FALLTHROUGH __attribute__((fallthrough))
#else
#   define IO_FALLTHROUGH ((void)0)
#endif // defined(IO_CXX_17)

// ------------- IO_NODISCARD / IO_CONSTEXPR / IO_CONSTEXPR_VAR ---------------
#ifdef IO_CXX_17
//   IO_NODISCARD --- [[nodiscard]] attribute (C++17+).
//      Prevents ignoring return values where it matters.
//      Falls back to empty on pre-C++17.
#   ifndef IO_NODISCARD
#       define IO_NODISCARD [[nodiscard]]
#   endif

//  IO_CONSTEXPR --- `constexpr` keyword (C++11+).
//      Falls back to inline on older standards.
//      Note: inline has existed since C++98 and is used for ODR-safety only.
#   ifndef IO_CONSTEXPR
#       define IO_CONSTEXPR constexpr // from C++17 or higher
#   endif

//  IO_CONSTEXPR_VAR --- `constexpr` variable (C++11+).
//     In C++17+, constexpr variables are implicitly inline.
//     In pre-C++17 mode, this macro resolves to 'const' and is intended
//     for use in templates only, where ODR violations do not occur.
#   ifndef IO_CONSTEXPR_VAR
#       define IO_CONSTEXPR_VAR constexpr // from C++17 or higher
#   endif
#else // fallback
#   ifndef IO_NODISCARD
#       define IO_NODISCARD // Falls back to empty on pre-C++17.
#   endif

    // IO_CONSTEXPR --- `inline` keyword (C++11+).
    // Rule:
    //   constexpr functions are implicitly inline.
    //   Do not add 'inline' explicitly unless constexpr is unavailable.
    // NOTE: 'inline' does not force inlining.
    //   It only relaxes the One Definition Rule (ODR) for header-only code.
#   ifndef IO_CONSTEXPR
#       define IO_CONSTEXPR inline // from C++98 to C++17
#   endif
#   ifndef IO_CONSTEXPR_VAR
#       define IO_CONSTEXPR_VAR const // from C++98 to C++17
#   endif
#endif // // Attribute/IO_CONSTEXPR defines
#pragma endregion // io_keywords


/*
    Usage:
        io::char_view name = IO_U8("тест_файл.txt");
        io::File f(name, io::OpenMode::Write);

    IO_U8("...") — UTF-8 string literal -> io::char_view (NO strlen, NO allocation)

    Why this exists:

    C++ has two different behaviors for UTF-8 literals:
    - C++17 and earlier:  u8"..." has type: const char[N]
    - C++20+:             u8"..." has type: const char8_t[N]

    Framework uses UTF-8 everywhere as `io::char_view`
    (pointer + explicit length, NOT zero-terminated).

    This helper:
    - converts both `char[N]` and `char8_t[N]`
    - keeps the length known at compile-time
    - avoids strlen()
    - avoids implicit encoding assumptions

    IMPORTANT:
    - This is SAFE: UTF-8 code units are 1 byte by definition.
    - reinterpret_cast is used ONLY to unify char8_t -> char.
    - The returned view is valid for the lifetime of the literal.
*/
#define IO_U8(s) ::io::u8view(u8##s)

#pragma endregion // macros

namespace io {
    // --- Includeless fixed-width integer types ---
    using i8 = signed char;
    using u8 = unsigned char;

    using i16 = short;
    using u16 = unsigned short;

    using i32 = int;
    using u32 = unsigned int;

    using i64 = long long;
    using u64 = unsigned long long;

    // --- Size and pointer-difference types (C++ freestanding replacements) ---
    using isize = decltype(static_cast<char*>(nullptr) - static_cast<char*>(nullptr));
    using usize = decltype(sizeof(0));

    struct u128 { u64 lo, hi; };

    IO_DEFINE_GLOBAL_ARRAY_AND_COUNT(out_buffer, char, IO_TERMINAL_BUFFER_SIZE)
    IO_DEFINE_GLOBAL_ARRAY_AND_COUNT(in_buffer, char, IO_TERMINAL_BUFFER_SIZE)
    static IO_CONSTEXPR_VAR usize npos = static_cast<usize>(-1);

    static inline float ceil(float x) noexcept { int i = (int)x; return (x > (float)i) ? (float)(i + 1) : (float)i; }
    IO_CONSTEXPR usize len(const char* s) noexcept {
        if (!s) return 0; // avoid UB by returning 0 for null pointers
        const char* p = s;
        while (*p != char(0)) ++p;
        return static_cast<usize>(p - s);
    }

    IO_NODISCARD static inline u128 add_u128(u128 a, u128 b) noexcept {
        u128 r{};
        r.lo = a.lo + b.lo;
        r.hi = a.hi + b.hi + (r.lo < a.lo);
        return r;
    }

    IO_NODISCARD static inline u128 add_u128_u64(u128 a, u64 b) noexcept {
        u128 r{};
        r.lo = a.lo + b;
        r.hi = a.hi + (r.lo < a.lo);
        return r;
    }
    IO_NODISCARD static inline u64 mul_u32(u32 a, u32 b) noexcept {
        return (u64)a * (u64)b; // 32x32->64, on x86 usually without __allmul
    }
    IO_NODISCARD static inline u128 mul_u64(u64 a, u64 b) noexcept {
#if defined(__SIZEOF_INT128__)
        unsigned __int128 p = (unsigned __int128)a * (unsigned __int128)b;
        return { (u64)p, (u64)(p >> 64) };

#elif (defined(_MSC_VER) && defined(IO_ARCH_X86_64))
        u128 r;
        r.lo = _umul128(a, b, &r.hi);
        return r;

#else
        // Pure portable 32x32->64 partials (MSVC x86 safe: no __allmul)
        const u32 a0 = (u32)a;
        const u32 a1 = (u32)(a >> 32);
        const u32 b0 = (u32)b;
        const u32 b1 = (u32)(b >> 32);

        const u64 p00 = (u64)a0 * (u64)b0;
        const u64 p01 = (u64)a0 * (u64)b1;
        const u64 p10 = (u64)a1 * (u64)b0;
        const u64 p11 = (u64)a1 * (u64)b1;

        u64 lo = p00;
        u64 hi = p11 + (p01 >> 32) + (p10 >> 32);

        { // add (p01 << 32)
            const u64 add = (p01 << 32);
            const u64 before = lo;
            lo = before + add;
            hi += (lo < before);
        }
        // add (p10 << 32)
        {
            const u64 add = (p10 << 32);
            const u64 before = lo;
            lo = before + add;
            hi += (lo < before);
        }
        return { lo, hi };
#endif
    }

    // minimal long division, but compact and readable
    static inline u32 div_u64_u32(u64 n, u32 d, u32* rem=nullptr) noexcept {
        if (d == 0) { if (rem) *rem=0; return 0; }
        u64 q = 0;
        u32 r = 0;
        for (int i=63; i>=0; --i) {
            r = (u32)((r<<1) | (u32)((n>>i) & 1ull));
            if (r>=d) { r-=d; q |= (1ull<<i); }
        }
        if (rem) *rem=r;
        return (u32)q;
    }

    static inline u8 f2u8(float x) noexcept {
        x = x < 0.f ? 0.f : (x > 1.f ? 1.f : x);
        float y = x * 255.f + 0.5f;
        // guarrantee range [0, 255], then cast<int>
        int iy = (int)y;
        if (iy < 0) iy = 0;
        if (iy > 255) iy = 255;
        return (u8)iy;
    }

#if !defined(IO_HAS_STD) && IO_ARCH_X86_32
    static inline u32 io_digit_to_u32_from_double_pos(double x) noexcept {
        // Preconditions: 0 <= x < 10
        u32 converted = 0;

#   if defined(_MSC_VER)
        unsigned short cw_old = 0;
        unsigned short cw_new = 0;
        __asm {
            fnstcw cw_old
            mov ax, cw_old
            or ax, 0x0C00         // round toward zero
            mov cw_new, ax
            fldcw cw_new

            fld qword ptr[x]
            fistp dword ptr[converted]

            fldcw cw_old
        }
#   elif defined(__GNUC__) || defined(__clang__)
        unsigned short cw_old = 0;
        unsigned short cw_new = 0;
        __asm__ __volatile__(
            "fnstcw %0\n\t"
            : "=m"(cw_old)
        );
        cw_new = (unsigned short)(cw_old | 0x0C00u);
        __asm__ __volatile__(
            "fldcw %0\n\t"
            "fldl  %1\n\t"
            "fistpl %2\n\t"
            "fldcw %3\n\t"
            :
        : "m"(cw_new), "m"(x), "m"(converted), "m"(cw_old)
            : "memory"
            );
#   else
#       error "Not implemented"
#   endif

        return converted;
    }

    static inline double io_digit_to_double(u32 d) noexcept {
        switch (d) {
        case 0: return 0.0;
        case 1: return 1.0;
        case 2: return 2.0;
        case 3: return 3.0;
        case 4: return 4.0;
        case 5: return 5.0;
        case 6: return 6.0;
        case 7: return 7.0;
        case 8: return 8.0;
        default:return 9.0;
        }
    }

    static inline bool io_double_is_nan(double x) noexcept {
        union { double d; u64 u; } v{ x };
        const u64 exp = (v.u >> 52) & 0x7FFull;
        const u64 frac = v.u & 0x000FFFFFFFFFFFFFull;
        return exp == 0x7FFull && frac != 0;
    }

    static inline bool io_double_is_inf(double x) noexcept {
        union { double d; u64 u; } v{ x };
        const u64 exp = (v.u >> 52) & 0x7FFull;
        const u64 frac = v.u & 0x000FFFFFFFFFFFFFull;
        return exp == 0x7FFull && frac == 0;
    }

#endif // !defined(IO_HAS_STD) && IO_ARCH_X86_32


    static inline void cpu_pause() noexcept {
#if defined(_MSC_VER) && defined(IO_ARCH_X86)
        _mm_pause(); // MSVC x86
#elif defined(IO_ARCH_X86)
        __asm__ __volatile__("pause"); // x86
#elif defined(_WIN32) || defined(_WIN64)
        ::YieldProcessor(); // Windows ARM
#else
        __asm__ __volatile__("yield"); // ARM
#endif
    }

    static inline void cpu_yield() noexcept {
#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32) || defined(_WIN64)
        ::SwitchToThread();
#   else
        ::sched_yield();
#   endif
#endif
    }

    struct Backoff {
        unsigned n = 0;

        void relax() noexcept {
            if (n < 16) {
                for (unsigned i = 0; i < (1u << n); ++i) cpu_pause();
            }
            else if (n < 22) {
                cpu_yield();
            }
            else {
#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32) || defined(_WIN64)
                ::Sleep(0);
#   else
                ::sched_yield();
#   endif
#endif
            }
            if (n < 30) ++n;
        }

        void reset() noexcept { n = 0; }
    };

    // ----------------- Basic SFINAE Primitives --------------------

    template<bool B, typename T = void> struct enable_if {};
    template<typename T> struct enable_if<true, T> { using type = T; };
    template<bool B, typename T = void>
    using enable_if_t = typename enable_if<B, T>::type;
    // ----------------- Compile-time constant wrapper ----------------
    //
    // constant<T, v>
    //   Minimal replacement for std::integral_constant.
    //   - Works in freestanding environments.
    //   - Does not depend on <type_traits>.
    //   - Safe in pre-C++17 due to template instantiation rules.
    // operator T()
    //   - constexpr since C++11.
    //   - inline fallback ensures header-only usage.
    template<typename T, T v>
    struct constant {
        static IO_CONSTEXPR_VAR T value = v;
        using type = constant;
        IO_CONSTEXPR operator T() const noexcept { return value; }
    };

    using true_t = constant<bool, true>;
    using false_t = constant<bool, false>;

    // ----------------------- is_same ------------------------------
    template<typename A, typename B> struct is_same : false_t {};
    template<typename A>             struct is_same<A, A> : true_t {};
    template<typename A, typename B> IO_CONSTEXPR_VAR bool is_same_v = is_same<A, B>::value;

    // ------------------------ void_t -----------------------------
    template<typename...>
    using void_t = void;
    template<typename T> struct is_void : false_t {};
    template<>           struct is_void<void> : true_t {};
    template<typename T> IO_CONSTEXPR_VAR bool is_void_v = is_void<T>::value;

    // -------- Convenience macro for enabling functions -----------
#define IO_REQUIRES(...) typename = enable_if_t<(__VA_ARGS__)>
#define IO_REQUIRES_T(...) enable_if_t<(__VA_ARGS__), int> = 0

    // -------------------- remove_reference --------------------
    template<typename T> struct remove_reference { using type = T; };
    template<typename T> struct remove_reference<T&> { using type = T; };
    template<typename T> struct remove_reference<T&&> { using type = T; };
    template<typename T> using remove_reference_t = typename remove_reference<T>::type;

    // ---------------------- move, forward ------------------------
    template<typename T> IO_CONSTEXPR remove_reference_t<T>&& move(T&& t) noexcept { return static_cast<remove_reference_t<T>&&>(t); }
    template<typename T> IO_CONSTEXPR T&& forward(remove_reference_t<T>& t) noexcept { return static_cast<T&&>(t); }
    template<typename T> IO_CONSTEXPR T&& forward(remove_reference_t<T>&& t) noexcept { return static_cast<T&&>(t); }

    // ============================================================
    //                   universal view (like span)
    // ============================================================
    template<typename T>
    struct view {
        using value_type = T;
        // -------------------- Constructors ---------------------------------
        // 1. Default
        IO_CONSTEXPR view() noexcept : _ptr(nullptr), _len(0) {}
        // 2a) Raw pointer + length
        IO_CONSTEXPR view(T* ptr, usize len) noexcept : _ptr(ptr), _len(len) {}
        // 2b) From C-string pointer (ONLY pointer, not array)
        template<typename U = T, typename P, IO_REQUIRES(is_same_v<U, const char> && is_same_v<P, const char*>)>
        IO_CONSTEXPR view(P ptr) noexcept : _ptr(ptr), _len(::io::len(ptr)) {}
        // 3a) From C-array (generic, but NOT for const char)
        template<usize N, typename U=T, IO_REQUIRES(!is_same_v<U, const char>)>
        IO_CONSTEXPR view(U(&arr)[N]) noexcept : _ptr(arr), _len(N) {}
        // 3b) From char literal / char array for const char: trim '\0' if present
        template<usize N, typename U=T, IO_REQUIRES(is_same_v<U, const char>)>
        IO_CONSTEXPR view(const char(&arr)[N]) noexcept : _ptr(arr), _len((N>0 && arr[N-1]=='\0') ? (N-1) : N) {}
        // -------------------- Access / Info -------------------------------
        IO_CONSTEXPR T& operator[](usize i) const noexcept { return _ptr[i]; }
        IO_CONSTEXPR T* data()    const noexcept { return _ptr; }
        IO_CONSTEXPR usize size() const noexcept { return _len; }
        IO_CONSTEXPR bool empty() const noexcept { return _len == 0; }
        inline operator bool() const noexcept { return data() && !empty(); }
        // -------------------- Iteration -----------------------------------
        IO_CONSTEXPR T* begin() const noexcept { return _ptr; }
        IO_CONSTEXPR T* end()   const noexcept { return _ptr + _len; }
        // -------------------- Convenience operations ----------------------
        IO_CONSTEXPR T& front() const noexcept { return _ptr[0]; }
        IO_CONSTEXPR T& back()  const noexcept { return _ptr[_len - 1]; }
        IO_CONSTEXPR view first(usize n) const noexcept { return view(_ptr, (n<=_len ? n : _len)); }
        IO_CONSTEXPR view last(usize n) const noexcept { return view(_ptr + (_len>n ? _len-n : 0), (n<=_len ? n : _len)); }
        IO_CONSTEXPR view drop(usize n) const noexcept { if (n >= _len) return view(); return view(_ptr+n, _len-n); }
        IO_CONSTEXPR view slice(usize pos, usize count) const noexcept {
            if (pos >= _len) return view();
            usize r = (_len - pos);
            if (count < r) r = count;
            return view(_ptr + pos, r);
        }
        IO_CONSTEXPR view subview(usize pos, usize count) const noexcept { return slice(pos, count); }
        // -------------------- find (string-like) --------------------------
        // 1) find single value
        IO_CONSTEXPR usize find(const T& value, usize pos = 0) const noexcept {
            if (pos >= _len) return npos;
            for (usize i = pos; i < _len; ++i) {
                if (_ptr[i] == value)
                    return i;
            }
            return npos;
        }
        // 2) find sub-view
        IO_CONSTEXPR usize find(view needle, usize pos = 0) const noexcept {
            if (needle._len == 0) return pos <= _len ? pos : npos;
            if (needle._len > _len || pos > (_len - needle._len)) return npos;

            for (usize i = pos; i <= (_len - needle._len); ++i) {
                usize j = 0;
                for (; j < needle._len; ++j) {
                    if (!(_ptr[i + j] == needle._ptr[j])) break;
                }
                if (j == needle._len) return i;
            }
            return npos;
        }
        // 3) find raw pointer + length (convenience)
        IO_CONSTEXPR usize find(const T* needle, usize needle_len, usize pos = 0) const noexcept {
            if (!needle) return npos;
            return find(view(needle, needle_len), pos);
        }
        // -------------------- Comparisons ---------------------------
        // view == view
        friend IO_CONSTEXPR bool operator==(view<T> a, view<T> b) noexcept {
            if (a._len != b._len) return false;
            for (usize i = 0; i < a._len; ++i) {
                if (!(a._ptr[i] == b._ptr[i])) return false;
            }
            return true;
        }
        friend IO_CONSTEXPR bool operator!=(view a, view b) noexcept { return !(a == b); }
        // char_view == literal   (only for const char views)
        template<usize N, typename U=T, IO_REQUIRES(is_same_v<U, const char>)>
        friend IO_CONSTEXPR bool operator==(view a, const char(&lit)[N]) noexcept { return a == view(lit); /* ctor already trims '\0' */ }
        template<usize N, typename U=T, IO_REQUIRES(is_same_v<U, const char>)>
        friend IO_CONSTEXPR bool operator!=(view a, const char(&lit)[N]) noexcept { return !(a == lit); }
        template<usize N, typename U = T, IO_REQUIRES(is_same_v<U, const char>)>
        friend IO_CONSTEXPR bool operator==(const char(&lit)[N], view b) noexcept { return b == lit; }
        template<usize N, typename U = T, IO_REQUIRES(is_same_v<U, const char>)>
        friend IO_CONSTEXPR bool operator!=(const char(&lit)[N], view b) noexcept { return !(b == lit); }
    protected:
        T* _ptr;
        usize _len;
    }; // struct view<T>

    template<typename T, usize N>
    struct view_n {
        view<T> v;
        static IO_CONSTEXPR_VAR usize size = N;

        view_n(view<T> other) noexcept : v{ other.data(), N} { }
        view_n(T* data) noexcept : v{ data, N } { }
        IO_CONSTEXPR T* data() const noexcept { return v.data(); }
        IO_CONSTEXPR usize len() const noexcept { return v.size(); }
        IO_CONSTEXPR operator view<T>() const noexcept { return v; }
    };
    template<usize N> IO_CONSTEXPR view_n<const u8, N> as_view(const u8(&a)[N]) noexcept { return {view<const u8>{a,N}}; }
    template<usize N> IO_CONSTEXPR view_n<const u8, N> as_view(view_n<u8, N> v) noexcept { return {view<const u8>{v.data(),N}}; }
    template<usize N> IO_CONSTEXPR view_n<u8, N> as_view_mut(u8(&a)[N]) noexcept { return {view<u8>{a,N}}; }

    // ---------------- convenience aliases -----------------------
    template<usize N> using char_view_n = view_n<const char, N>;
    template<usize N> using char_view_mut_n = view_n<char, N>;
    using char_view     = view<const char>;
    using char_view_mut = view<char>;

    template<usize N> using byte_view_n     = view_n<const u8, N>;
    template<usize N> using byte_view_mut_n = view_n<u8, N>;
    using byte_view     = view<const u8>;
    using byte_view_mut = view<u8>;
   
    template<usize N>
    IO_NODISCARD IO_CONSTEXPR char_view u8view(const char(&s)[N]) noexcept {
        return char_view{ s, N ? N-1 : 0 }; // N includes '\0'
    }

#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
    template<usize N>
    IO_NODISCARD IO_CONSTEXPR char_view u8view(const char8_t(&s)[N]) noexcept {
        return char_view{ reinterpret_cast<const char*>(s), N ? N-1 : 0 };
    }
#endif
} // namespace io


// ============================================================================
//                         A T O M I C
// ============================================================================

#pragma region atomic
// Minimal freestanding-friendly atomic wrapper for C++20
// - Prefer std::atomic if available
// - Fall back to GCC/Clang __atomic builtins if available
// - Fall back to older __sync builtins if available
// - Final fallback: volatile-backed (NO ATOMIC GUARANTEES!)

#if defined(__has_include)
#  if __has_include(<atomic>)
#    define IO_HAS_STD_ATOMIC 1
#  else
#    define IO_HAS_STD_ATOMIC 0
#  endif
#else
#  define IO_HAS_STD_ATOMIC 0
#endif

// Allow user to force disable std::atomic usage
#if defined(IO_NO_STD_ATOMIC)
#  undef IO_HAS_STD_ATOMIC
#  define IO_HAS_STD_ATOMIC 0
#endif

#if IO_HAS_STD_ATOMIC
#  include <atomic>
#endif

// Compiler builtins detection
#if !IO_HAS_STD_ATOMIC
#  if defined(__GNUC__) || defined(__clang__)
#    define IO_HAS___ATOMIC_BUILTINS 1
#  else
#    define IO_HAS___ATOMIC_BUILTINS 0
#  endif
#endif

#if !IO_HAS_STD_ATOMIC && !defined(IO_HAS___ATOMIC_BUILTINS)
#  define IO_HAS___ATOMIC_BUILTINS 0
#endif

#if !IO_HAS_STD_ATOMIC && !IO_HAS___ATOMIC_BUILTINS
#  if defined(__GNUC__) || defined(__clang__)
#    define IO_HAS___SYNC_BUILTINS 1
#  else
#    define IO_HAS___SYNC_BUILTINS 0
#  endif
#else
#  define IO_HAS___SYNC_BUILTINS 0
#endif

namespace io {

    // Simple memory_order enum for API parity. If std::atomic is present we'll use std::memory_order.
#if IO_HAS_STD_ATOMIC
    using memory_order = std::memory_order;
    static IO_CONSTEXPR_VAR memory_order memory_order_relaxed = std::memory_order_relaxed;
    static IO_CONSTEXPR_VAR memory_order memory_order_consume = std::memory_order_consume;
    static IO_CONSTEXPR_VAR memory_order memory_order_acquire = std::memory_order_acquire;
    static IO_CONSTEXPR_VAR memory_order memory_order_release = std::memory_order_release;
    static IO_CONSTEXPR_VAR memory_order memory_order_acq_rel = std::memory_order_acq_rel;
    static IO_CONSTEXPR_VAR memory_order memory_order_seq_cst = std::memory_order_seq_cst;
#else
// lightweight stand-in (only names; semantics depend on backend)
    enum memory_order {
        memory_order_relaxed = 0,
        memory_order_consume,
        memory_order_acquire,
        memory_order_release,
        memory_order_acq_rel,
        memory_order_seq_cst
    };
#endif

    // Primary template
    template<typename T>
    class atomic {
    public:
#if IO_HAS_STD_ATOMIC
        // Use std::atomic<T> directly
        atomic() noexcept = default;
        IO_CONSTEXPR atomic(T v) noexcept : _a(v) {}
        atomic(const atomic&) = delete;
        atomic& operator=(const atomic&) = delete;

        IO_NODISCARD inline T load(memory_order mo = memory_order_seq_cst) const noexcept {
            return _a.load(mo);
        }

        inline void store(T v, memory_order mo = memory_order_seq_cst) noexcept {
            _a.store(v, mo);
        }

        IO_NODISCARD inline T exchange(T v, memory_order mo = memory_order_seq_cst) noexcept {
            return _a.exchange(v, mo);
        }

        IO_NODISCARD inline bool compare_exchange_strong(T& expected, T desired,
            memory_order success = memory_order_seq_cst,
            memory_order failure = memory_order_seq_cst) noexcept {
            return _a.compare_exchange_strong(expected, desired, success, failure);
        }

        IO_NODISCARD inline T fetch_add(T v, memory_order mo = memory_order_seq_cst) noexcept {
            return _a.fetch_add(v, mo);
        }

        IO_NODISCARD inline T fetch_sub(T v, memory_order mo = memory_order_seq_cst) noexcept {
            return _a.fetch_sub(v, mo);
        }

        IO_NODISCARD inline T fetch_and(T v, memory_order mo = memory_order_seq_cst) noexcept {
            return _a.fetch_and(v, mo);
        }

        IO_NODISCARD inline T fetch_or(T v, memory_order mo = memory_order_seq_cst) noexcept {
            return _a.fetch_or(v, mo);
        }

        // convenience
        operator T() const noexcept { return load(); }
        T operator=(T v) noexcept { store(v); return v; }

    private:
        std::atomic<T> _a;
#else
        // Non-std fallback: try to use __atomic builtins, then __sync, else volatile fallback.
        inline atomic() noexcept : _v() {}
        IO_CONSTEXPR atomic(T v) noexcept : _v(v) {}

        atomic(const atomic&) = delete;
        atomic& operator=(const atomic&) = delete;

        IO_NODISCARD inline T load(memory_order /*mo*/ = memory_order_seq_cst) const noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_load_n(&_v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            // __sync builtins do not have load; use read of volatile (not strictly atomic)
            return _v;
#else
            // WARNING: not atomic
            return _v;
#endif
        }

        inline void store(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            __atomic_store_n(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            // no direct store builtin; use assignment (may not be atomic)
            _v = v;
#else
            _v = v; // not atomic
#endif
        }

        IO_NODISCARD inline T exchange(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_exchange_n(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            // emulate with loop
            T old;
            do {
                old = _v;
            } while (!__sync_bool_compare_and_swap(&_v, old, v));
            return old;
#else
            T old = _v;
            _v = v;
            return old;
#endif
        }

        IO_NODISCARD inline bool compare_exchange_strong(T& expected, T desired,
            memory_order /*success*/ = memory_order_seq_cst,
            memory_order /*failure*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_compare_exchange_n(&_v, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            // __sync_bool_compare_and_swap returns bool
            return __sync_bool_compare_and_swap(&_v, expected, desired)
                ? (true)
                : ((expected = _v), false);
#else
            // Not atomic: only succeed if exact match
            if (_v == expected) {
                _v = desired;
                return true;
            }
            else {
                expected = _v;
                return false;
            }
#endif
        }

        IO_NODISCARD inline T fetch_add(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_fetch_add(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            return __sync_fetch_and_add(&_v, v);
#else
            T old = _v;
            _v = old + v;
            return old;
#endif
        }

        IO_NODISCARD inline T fetch_sub(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_fetch_sub(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            return __sync_fetch_and_sub(&_v, v);
#else
            T old = _v;
            _v = old - v;
            return old;
#endif
        }

        IO_NODISCARD inline T fetch_and(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_fetch_and(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            return __sync_fetch_and_and(&_v, v); // rarely available, keep for completeness
#else
            T old = _v;
            _v = old & v;
            return old;
#endif
        }

        IO_NODISCARD inline T fetch_or(T v, memory_order /*mo*/ = memory_order_seq_cst) noexcept {
#if IO_HAS___ATOMIC_BUILTINS
            return __atomic_fetch_or(&_v, v, __ATOMIC_SEQ_CST);
#elif IO_HAS___SYNC_BUILTINS
            return __sync_fetch_and_or(&_v, v);
#else
            T old = _v;
            _v = old | v;
            return old;
#endif
        }

        operator T() const noexcept { return load(); }
        T operator=(T v) noexcept { store(v); return v; }

    private:
        // Use plain (volatile) storage for fallback (may not be atomic)
        volatile T _v;
#endif // IO_HAS_STD_ATOMIC
    }; // class atomic

    struct IO_ALIGN(IO_CACHELINE) PaddedSizeT {
        io::atomic<size_t> v{ 0 };
        unsigned char pad[IO_CACHELINE - sizeof(io::atomic<size_t>) > 0
            ? IO_CACHELINE - sizeof(io::atomic<size_t>) : 1];
    };

} // namespace io


#pragma endregion // atomic


// ============================================================================
//                         T H R E A D
// ============================================================================

namespace io {
    // ============================================================================
    // Thread
    // ============================================================================
    struct Thread {
        using Fn = void(*)(void*);

        Thread() noexcept = default;
        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        ~Thread() noexcept {
            if (_running) join();
        }

        bool start(Fn fn, void* arg = nullptr, unsigned stack_size = 0) noexcept {
            if (!fn || _running) return false;
            _fn = fn; _arg = arg;

#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32) || defined(_WIN64)
            DWORD tid = 0;
            _h = ::CreateThread(nullptr, (SIZE_T)stack_size, &Thread::_win_entry, this, 0, &tid);
            if (!_h) return false;
            _running = true;
            return true;
#   else
            pthread_attr_t attr;
            if (pthread_attr_init(&attr) != 0) return false;
            if (stack_size) (void)pthread_attr_setstacksize(&attr, (size_t)stack_size);
            int rc = pthread_create(&_t, &attr, &Thread::_posix_entry, this);
            (void)pthread_attr_destroy(&attr);
            if (rc != 0) return false;
            _running = true;
            return true;
#   endif
#endif // IO_IMPLEMENTATION
        }

        bool join() noexcept {
            if (!_running) return false;
#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32) || defined(_WIN64)
            ::WaitForSingleObject(_h, INFINITE);
            ::CloseHandle(_h);
            _h = nullptr;
#   else
            (void)pthread_join(_t, nullptr);
#   endif
#endif // IO_IMPLEMENTATION
            _running = false;
            return true;
        }

        bool detach() noexcept {
            if (!_running) return false;
#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32) || defined(_WIN64)
            ::CloseHandle(_h);
            _h = nullptr;
#   else
            (void)pthread_detach(_t);
#   endif
#endif // IO_IMPLEMENTATION
            _running = false;
            return true;
    }

        bool running() const noexcept { return _running; }

        static unsigned max_workers() noexcept {
#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32) || defined(_WIN64)
            DWORD n = ::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
            return n ? (unsigned)n : 1u;
#   else
            long n = ::sysconf(_SC_NPROCESSORS_ONLN);
            return (n > 0) ? (unsigned)n : 1u;
#   endif
#endif // IO_IMPLEMENTATION
        }

    private:
#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32) || defined(_WIN64)
        static DWORD WINAPI _win_entry(LPVOID p) noexcept {
            Thread* self = (Thread*)p;
            self->_fn(self->_arg);
            return 0;
        }
        HANDLE _h = nullptr;
#   else
        static void* _posix_entry(void* p) noexcept {
            Thread* self = (Thread*)p;
            self->_fn(self->_arg);
            return nullptr;
        }
        pthread_t _t{};
#   endif
#endif // IO_IMPLEMENTATION

        Fn _fn = nullptr;
        void* _arg = nullptr;
        bool _running = false;
    }; // struct Thread

    struct ThreadPool {
        using TaskFn = void(*)(void*);

        struct Task {
            TaskFn fn;
            void* arg;
        };

        ThreadPool() noexcept = default;
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool() noexcept {
            if (_inited) shutdown(true);
        }

        // cap MUST be power-of-two (2^k): recommended >= workers*4
        bool init(unsigned workers, unsigned cap_pow2 = 1024) noexcept {
            if (_inited || workers == 0 || cap_pow2 == 0 || !is_pow2_(cap_pow2)) return false;

            _cap = cap_pow2;
            _mask = _cap - 1;

            _cells = (Cell*)::operator new(sizeof(Cell) * (size_t)_cap, std::nothrow);
            if (!_cells) return false;

            // init sequence numbers
            for (size_t i = 0; i < (size_t)_cap; ++i) {
                // placement-new Cell (trivial, but let it be correct for atomics)
                new (&_cells[i]) Cell();
                _cells[i].seq.store(i, memory_order_relaxed);
            }

            _enq.v.store(0, memory_order_relaxed);
            _deq.v.store(0, memory_order_relaxed);
            _stop.v.store(0, memory_order_relaxed);

            _threads = (Thread*)::operator new(sizeof(Thread) * workers, std::nothrow);
            if (!_threads) { destroy_queue_(); return false; }

            _n = workers;
            _inited = true;

            for (unsigned i = 0; i < _n; ++i) {
                if (!_threads[i].start(&ThreadPool::_worker_entry, this)) {
                    shutdown(true);
                    return false;
                }
            }
            return true;
        }

        // non-blocking: false if stop or full
        bool submit(TaskFn fn, void* arg = nullptr) noexcept {
            if (!_inited || !fn) return false;
            if (_stop.v.load(memory_order_relaxed)) return false;
            Task t{ fn, arg };
            return enqueue_(t);
        }

        // Graceful: stop=1, workers will finish the queue and quit
        void shutdown(bool wait = true) noexcept {
            if (!_inited) return;

            _stop.v.store(1, memory_order_release);

            if (wait) {
                for (unsigned i = 0; i < _n; ++i) (void)_threads[i].join();
            }
            else {
                for (unsigned i = 0; i < _n; ++i) (void)_threads[i].detach();
            }

            if (_threads) { ::operator delete(_threads); _threads = nullptr; }
            destroy_queue_();

            _n = 0; _cap = 0; _mask = 0;
            _inited = false;
            }

        unsigned worker_count() const noexcept { return _n; }
        unsigned capacity() const noexcept { return _cap; }

    private:
        struct Cell {
            io::atomic<size_t> seq{ 0 };
            Task task{ nullptr, nullptr };
        };

        static void _worker_entry(void* p) noexcept {
            ((ThreadPool*)p)->worker_loop_();
        }

        void worker_loop_() noexcept {
            Task t{ nullptr, nullptr };
            Backoff b;
            for (;;) {
                if (dequeue_(t)) {
                    b.reset();
                    t.fn(t.arg);
                    continue;
                }

                // queue empty
                if (_stop.v.load(memory_order_acquire)) {
                    // guarantee that it is empty fr: double-check
                    if (!dequeue_(t)) return;
                    // if suddenly appears — do and continue
                    b.reset();
                    t.fn(t.arg);
                    continue;
                }

                b.relax();
            }
        }

        bool enqueue_(const Task& t) noexcept {
            size_t pos = _enq.v.load(memory_order_relaxed);

            for (;;) {
                Cell* c = &_cells[pos & _mask];
                size_t seq = c->seq.load(memory_order_acquire);
                intptr_t dif = (intptr_t)seq - (intptr_t)pos;

                if (dif == 0) {
                    size_t expected = pos;
                    if (_enq.v.compare_exchange_strong(expected, pos + 1,
                        memory_order_relaxed, memory_order_relaxed)) {
                        c->task = t;                         // publish data
                        c->seq.store(pos + 1, memory_order_release);
                        return true;
                }
                    pos = expected;
                }
                else if (dif < 0) {
                    return false; // full
                }
                else {
                    pos = _enq.v.load(memory_order_relaxed);
                }
            }
        }

        bool dequeue_(Task& out) noexcept {
            size_t pos = _deq.v.load(memory_order_relaxed);

            for (;;) {
                Cell* c = &_cells[pos & _mask];
                size_t seq = c->seq.load(memory_order_acquire);
                intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);

                if (dif == 0) {
                    size_t expected = pos;
                    if (_deq.v.compare_exchange_strong(expected, pos + 1,
                        memory_order_relaxed, memory_order_relaxed)) {
                        out = c->task;                       // consume data
                        c->seq.store(pos + _cap, memory_order_release);
                        return true;
                    }
                    pos = expected;
                }
                else if (dif < 0) {
                    return false; // empty
                }
                else {
                    pos = _deq.v.load(memory_order_relaxed);
                }
            }
        }

        static bool is_pow2_(unsigned x) noexcept {
            return (x & (x - 1u)) == 0u;
        }

        void destroy_queue_() noexcept {
            if (_cells) {
                // Cell is trivial, call destructor anyway for sure
                ::operator delete(_cells);
                _cells = nullptr;
            }
        }

    private:
        Thread* _threads = nullptr;
        unsigned _n = 0;

        Cell* _cells = nullptr;
        unsigned _cap = 0;
        unsigned _mask = 0;

        PaddedSizeT _enq{ 0 };
        PaddedSizeT _deq{ 0 };
        struct IO_ALIGN(IO_CACHELINE) StopFlag {
            io::atomic<int> v{ 0 };
            unsigned char pad[IO_CACHELINE - sizeof(io::atomic<int>) > 0
                ? IO_CACHELINE - sizeof(io::atomic<int>) : 1];
        } _stop;

        bool _inited = false;
    }; // struct ThreadPool
} // namespace io

// ============================================================================
//                           S Y S C A L L S
// ============================================================================

namespace io {
    // --- Allocate ---
    void* alloc(usize bytes) noexcept;
    void   free(void* ptr) noexcept;

    // --- Process ---
#if defined(__linux__)
    [[noreturn]]
#endif
    void exit_process(int error_code) noexcept;

    // --- Sleep / Time ---
    void sleep_ms(unsigned ms) noexcept;
    IO_NODISCARD inline u64 monotonic_ms() noexcept;
    IO_NODISCARD inline u64 monotonic_us() noexcept;

    // --- Crypto hygiene ---
    void secure_zero(void* p, usize size) noexcept;

    // Fill [dst, dst+size) with OS-provided entropy. Returns false on failure.
    bool os_entropy(void* dst, usize size) noexcept;

namespace native {
#if defined(_WIN32)
    // nothing to declare
#elif defined(__linux__)
    struct timespec {
        long tv_sec;
        long tv_nsec;
    };
    // ---- syscall numbers ----
#if defined(IO_ARCH_X86_64)
    static IO_CONSTEXPR_VAR long k_sys_read        = 0;
    static IO_CONSTEXPR_VAR long k_sys_write       = 1;
    static IO_CONSTEXPR_VAR long k_sys_openat      = 257;
    static IO_CONSTEXPR_VAR long k_sys_close       = 3;
    static IO_CONSTEXPR_VAR long k_sys_lseek       = 8;
    static IO_CONSTEXPR_VAR long k_sys_fsync       = 74;
    static IO_CONSTEXPR_VAR long k_sys_newfstatat  = 262;
    static IO_CONSTEXPR_VAR long k_sys_unlinkat    = 263;
    static IO_CONSTEXPR_VAR long k_sys_renameat    = 264;
    static IO_CONSTEXPR_VAR long k_sys_mkdirat     = 258;
    static IO_CONSTEXPR_VAR long k_sys_getcwd      = 79;
    static IO_CONSTEXPR_VAR long k_sys_getdents64  = 217;
    static IO_CONSTEXPR_VAR long k_sys_mmap          = 9;
    static IO_CONSTEXPR_VAR long k_sys_munmap        = 11;
    static IO_CONSTEXPR_VAR long k_sys_exit          = 60;
    static IO_CONSTEXPR_VAR long k_sys_nanosleep     = 35;
    static IO_CONSTEXPR_VAR long k_sys_clock_gettime = 228;
#elif defined(IO_ARCH_X86_32)
    static IO_CONSTEXPR_VAR long k_sys_read        = 3;
    static IO_CONSTEXPR_VAR long k_sys_write       = 4;
    static IO_CONSTEXPR_VAR long k_sys_openat      = 295;
    static IO_CONSTEXPR_VAR long k_sys_close       = 6;
    static IO_CONSTEXPR_VAR long k_sys_lseek       = 19;
    static IO_CONSTEXPR_VAR long k_sys_fsync       = 118;
    static IO_CONSTEXPR_VAR long k_sys_newfstatat  = 300; // fstatat64 on i386 via newfstatat number
    static IO_CONSTEXPR_VAR long k_sys_unlinkat    = 301;
    static IO_CONSTEXPR_VAR long k_sys_renameat    = 302;
    static IO_CONSTEXPR_VAR long k_sys_mkdirat     = 296;
    static IO_CONSTEXPR_VAR long k_sys_getcwd      = 183;
    static IO_CONSTEXPR_VAR long k_sys_getdents64  = 220;
    static IO_CONSTEXPR_VAR long k_sys_mmap2         = 192; // i386
    static IO_CONSTEXPR_VAR long k_sys_munmap        = 91;
    static IO_CONSTEXPR_VAR long k_sys_exit          = 1;
    static IO_CONSTEXPR_VAR long k_sys_nanosleep     = 162;
    static IO_CONSTEXPR_VAR long k_sys_clock_gettime = 265; // i386
#else
#   error "linux: unsupported arch"
#endif

    static IO_CONSTEXPR_VAR int k_at_fdcwd = -100;
    static IO_CONSTEXPR_VAR int k_at_removedir = 0x200;
    static IO_CONSTEXPR_VAR int k_at_symlink_nofollow = 0x100;

    // mmap flags/prot
    static IO_CONSTEXPR_VAR int k_prot_read  = 0x1;
    static IO_CONSTEXPR_VAR int k_prot_write = 0x2;
    static IO_CONSTEXPR_VAR int k_map_private   = 0x02;
    static IO_CONSTEXPR_VAR int k_map_anonymous = 0x20;

    // open flags (kernel ABI)
    static IO_CONSTEXPR_VAR int k_o_rdonly   = 0;
    static IO_CONSTEXPR_VAR int k_o_wronly   = 1;
    static IO_CONSTEXPR_VAR int k_o_rdwr     = 2;
    static IO_CONSTEXPR_VAR int k_o_creat    = 0x40;
    static IO_CONSTEXPR_VAR int k_o_trunc    = 0x200;
    static IO_CONSTEXPR_VAR int k_o_append   = 0x400;
    static IO_CONSTEXPR_VAR int k_o_cloexec  = 0x80000;
    static IO_CONSTEXPR_VAR int k_o_directory= 0x10000;

    static IO_CONSTEXPR_VAR int k_seek_set = 0;
    static IO_CONSTEXPR_VAR int k_seek_cur = 1;
    static IO_CONSTEXPR_VAR int k_seek_end = 2;

    // errno values we care about
    static IO_CONSTEXPR_VAR long k_erange     = 34;
    static IO_CONSTEXPR_VAR long k_eagain     = 11;
    static IO_CONSTEXPR_VAR long k_ewouldblock= 11; // same as EAGAIN on Linux
    static IO_CONSTEXPR_VAR long k_eintr      = 4;
    static IO_CONSTEXPR_VAR long k_econnreset = 104;

    // ---- raw syscalls (return: >=0 or -errno) ----
#if defined(IO_ARCH_X86_64)
    static inline long sys0(long n) noexcept {
        long r; asm volatile("syscall" : "=a"(r) : "a"(n) : "rcx","r11","memory"); return r;
    }
    static inline long sys1(long n, long a1) noexcept {
        long r; asm volatile("syscall" : "=a"(r) : "a"(n), "D"(a1) : "rcx","r11","memory"); return r;
    }
    static inline long sys2(long n, long a1, long a2) noexcept {
        long r; asm volatile("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2) : "rcx","r11","memory"); return r;
    }
    static inline long sys3(long n, long a1, long a2, long a3) noexcept {
        long r; asm volatile("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2), "d"(a3) : "rcx","r11","memory"); return r;
    }
    static inline long sys4(long n, long a1, long a2, long a3, long a4) noexcept {
        long r;
        register long r10 asm("r10") = a4;
        asm volatile("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10) : "rcx","r11","memory");
        return r;
    }
    static inline long sys6(long n, long a1, long a2, long a3, long a4, long a5, long a6) noexcept {
        long r;
        register long r10 asm("r10") = a4;
        register long r8  asm("r8")  = a5;
        register long r9  asm("r9")  = a6;
        asm volatile("syscall"
            : "=a"(r)
            : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
            : "rcx","r11","memory");
        return r;
    }
#elif defined(IO_ARCH_X86_32)
    static inline long sys0(long n) noexcept {
        long r; asm volatile("int $0x80" : "=a"(r) : "0"(n) : "memory"); return r;
    }
    static inline long sys1(long n, long a1) noexcept {
        long r; asm volatile("int $0x80" : "=a"(r) : "0"(n), "b"(a1) : "memory"); return r;
    }
    static inline long sys2(long n, long a1, long a2) noexcept {
        long r; asm volatile("int $0x80" : "=a"(r) : "0"(n), "b"(a1), "c"(a2) : "memory"); return r;
    }
    static inline long sys3(long n, long a1, long a2, long a3) noexcept {
        long r; asm volatile("int $0x80" : "=a"(r) : "0"(n), "b"(a1), "c"(a2), "d"(a3) : "memory"); return r;
    }
    static inline long sys4(long n, long a1, long a2, long a3, long a4) noexcept {
        long r; asm volatile("int $0x80" : "=a"(r) : "0"(n), "b"(a1), "c"(a2), "d"(a3), "S"(a4) : "memory"); return r;
    }
    static inline long sys6(long n, long a1, long a2, long a3, long a4, long a5, long a6) noexcept {
        long r;
        asm volatile("int $0x80"
            : "=a"(r)
            : "0"(n), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5)
            : "memory");
        // i386 has no easy 6th reg here without stack; for mmap2 we’ll use a different syscall.
        return r;
    }
#else
#   error "freestanding: unsupported arch"
#endif

    static inline bool is_err(long r) noexcept { return r < 0; }
    static inline long err_no(long r) noexcept { return -r; }

// ------------------------- raw linux syscalls (no libc) -------------------------
    // Returns: >=0 bytes written, or negative -errno.
    static inline long linux_sys_write(int fd, const void* buf, unsigned long n) noexcept {
#if defined(IO_ARCH_X86_64)
        long ret;
        asm volatile( // k_sys_write = 1 on x86_64
            "syscall"
            : "=a"(ret)
            : "a"(k_sys_write), "D"(fd), "S"(buf), "d"(n)
            : "rcx", "r11", "memory"
        );
        return ret;
#elif defined(IO_ARCH_X86_32)
        long ret;
        asm volatile( // k_sys_write = 4 on i386, int 0x80 ABI
            "int $0x80"
            : "=a"(ret)
            : "0"(k_sys_write), "b"(fd), "c"(buf), "d"(n)
            : "memory"
        );
        return ret;
#else
#   error "linux_sys_write: unsupported arch (add syscall ABI)"
#endif
    }

    static inline long linux_sys_read(int fd, void* buf, unsigned long n) noexcept {
#if defined(IO_ARCH_X86_64)
        long ret;
        asm volatile( // k_sys_read = 0 on x86_64
            "syscall"
            : "=a"(ret)
            : "a"(k_sys_read), "D"(fd), "S"(buf), "d"(n)
            : "rcx", "r11", "memory"
        );
        return ret;
#elif defined(IO_ARCH_X86_32)
        long ret;
        asm volatile( // k_sys_read = 3 on i386
            "int $0x80"
            : "=a"(ret)
            : "0"(k_sys_read), "b"(fd), "c"(buf), "d"(n)
            : "memory"
        );
        return ret;
#else
#   error "linux_sys_read: unsupported arch"
#endif
    }
#endif // __linux__
} // namespace native


    // ========================================================================
    //                         M U T E X  ,  A L I G N
    // ========================================================================

    // ------------------------- spin_mutex -------------------------------
    struct spin_mutex {
        atomic<u32> state{ 0 };
        void lock() noexcept {
            Backoff b;
            for (;;) {
                if (state.load(memory_order_relaxed) == 0 &&
                    state.exchange(1, memory_order_acq_rel) == 0)
                    return;
                b.relax();
            }
        }
        void unlock() noexcept { state.store(0, memory_order_release); }
    };

    namespace global {
        static spin_mutex heap_lock;
    } // namespace global


    // ------------------------- aligned alloc ----------------------------
    // Notes:
    // - io::alloc(total) returns a pointer we must pass back to io::free().
    // - For aligned allocations we return an adjusted pointer, so we store the base
    //   pointer right before the aligned address.
    struct alloc_header {
        void* base;
    };
    static inline void* alloc_aligned(usize payload_bytes, usize alignment) noexcept {
        if (alignment < alignof(void*)) alignment = alignof(void*);
        if ((alignment & (alignment-1)) != 0) return nullptr; // power-of-two required

        const usize extra = (alignment-1) + sizeof(alloc_header);
        if (payload_bytes > (usize)-1 - extra) return nullptr; // overflow guard
        const usize total = payload_bytes + extra;

        global::heap_lock.lock();
        void* base = io::alloc(total);
        global::heap_lock.unlock();
        if (!base) return nullptr;

        const uintptr_t start = (uintptr_t)base + sizeof(alloc_header);
        const uintptr_t aligned = (start + (alignment-1)) & ~(uintptr_t)(alignment-1);

        auto* hdr = reinterpret_cast<alloc_header*>(aligned - sizeof(alloc_header));
        hdr->base = base;
        return reinterpret_cast<void*>(aligned);
    }
    static inline void free_aligned(void* p) noexcept {
        if (!p) return;

        auto* hdr = reinterpret_cast<alloc_header*>((uintptr_t)p - sizeof(alloc_header));
        void* base = hdr->base;

        global::heap_lock.lock();
        io::free(base);
        global::heap_lock.unlock();
    }
    template<class T, class... Args>
    IO_NODISCARD static inline T* heap_new(Args&&... args) noexcept {
        void* mem = io::alloc_aligned(sizeof(T), alignof(T));
        if (!mem) return nullptr;
        return new (mem) T(static_cast<Args&&>(args)...); // placement-new
    }
    template<class T>
    static inline void heap_delete(T* p) noexcept {
        if (!p) return;
        p->~T();                 // explicit dtor
        io::free_aligned(p);     // our allocator
    }

    // ========================================================================
    //                    S M A R T   P O I N T E R S
    // ========================================================================

    // ------------------------- unique_ptr -------------------------------
    // Owns a single object allocated by io::alloc_aligned().
    template<class T>
    struct unique_ptr {
        T* p{ nullptr };

        IO_CONSTEXPR unique_ptr() noexcept = default;
        explicit unique_ptr(T* x) noexcept : p(x) {}

        unique_ptr(const unique_ptr&) = delete;
        unique_ptr& operator=(const unique_ptr&) = delete;

        unique_ptr(unique_ptr&& o) noexcept : p(o.p) { o.p = nullptr; }
        unique_ptr& operator=(unique_ptr&& o) noexcept {
            if (this != &o) { reset(nullptr); p = o.p; o.p = nullptr; }
            return *this;
        }

        ~unique_ptr() noexcept { reset(nullptr); }

        IO_NODISCARD T* get() const noexcept { return p; }
        IO_NODISCARD T& operator*() const noexcept { return *p; }
        IO_NODISCARD T* operator->() const noexcept { return p; }
        explicit operator bool() const noexcept { return p != nullptr; }

        T* release() noexcept { T* r = p; p = nullptr; return r; }

        void reset(T* x) noexcept {
            if (p) {
                p->~T();
                io::free_aligned(p);
            }
            p = x;
        }
    };

    template<class T, class... Args>
    IO_NODISCARD static inline unique_ptr<T> make_unique(Args&&... args) noexcept {
        void* mem = io::alloc_aligned(sizeof(T), alignof(T));
        if (!mem) return unique_ptr<T>{nullptr};
        T* obj = new (mem) T(static_cast<Args&&>(args)...);
        return unique_ptr<T>{obj};
    }

    // ------------------------- unique_bytes -----------------------------
    // Owns a raw byte buffer allocated by io::alloc_aligned().
    struct unique_bytes {
        u8* p{ nullptr };

        IO_CONSTEXPR unique_bytes() noexcept = default;
        explicit unique_bytes(u8* x) noexcept : p(x) {}

        unique_bytes(const unique_bytes&) = delete;
        unique_bytes& operator=(const unique_bytes&) = delete;

        unique_bytes(unique_bytes&& o) noexcept : p(o.p) { o.p = nullptr; }
        unique_bytes& operator=(unique_bytes&& o) noexcept {
            if (this != &o) { reset(nullptr); p = o.p; o.p = nullptr; }
            return *this;
        }

        ~unique_bytes() noexcept { reset(nullptr); }

        IO_NODISCARD u8* get() const noexcept { return p; }
        explicit operator bool() const noexcept { return p != nullptr; }

        u8* release() noexcept { u8* r = p; p = nullptr; return r; }

        void reset(u8* x) noexcept {
            if (p) io::free_aligned(p);
            p = x;
        }
    };

    // ========================================================================
    //                         C O N T A I N E R S
    // ========================================================================
    // ------------------------- vector ----------------------------------
    // Uses io::alloc_aligned() to support over-aligned T safely.
    // Thread-safety: same as std::vector (external synchronization required).
    template<class T>
    struct vector {
        using value_type = T;

        IO_CONSTEXPR vector() noexcept = default;

        ~vector() noexcept {
            destroy_range(_ptr, _len);
            _raw.reset(nullptr);
            _ptr = nullptr;
            _len = 0;
            _cap = 0;
        }

        vector(const vector&) = delete;
        vector& operator=(const vector&) = delete;

        vector(vector&& o) noexcept {
            _raw = static_cast<unique_bytes&&>(o._raw);
            _ptr = o._ptr;  o._ptr = nullptr;
            _len = o._len;  o._len = 0;
            _cap = o._cap;  o._cap = 0;
        }

        vector& operator=(vector&& o) noexcept {
            if (this == &o) return *this;
            this->~vector();
            new (this) vector(static_cast<vector&&>(o));
            return *this;
        }

        // ---- view conversion ----
        IO_NODISCARD view<T> as_view() noexcept { return view<T>(_ptr, _len); }
        IO_NODISCARD view<const T> as_view() const noexcept { return view<const T>(_ptr, _len); }

        // ---- size/capacity ----
        IO_NODISCARD usize size() const noexcept { return _len; }
        IO_NODISCARD usize capacity() const noexcept { return _cap; }
        IO_NODISCARD bool empty() const noexcept { return _len == 0; }

        // ---- iterators ----
        IO_NODISCARD T* begin() noexcept { return _ptr; }
        IO_NODISCARD T* end() noexcept { return _ptr + _len; }
        IO_NODISCARD const T* begin() const noexcept { return _ptr; }
        IO_NODISCARD const T* end() const noexcept { return _ptr + _len; }

        // ---- data ----
        IO_NODISCARD T* data() noexcept { return _ptr; }
        IO_NODISCARD const T* data() const noexcept { return _ptr; }

        // ---- element access ----
        IO_NODISCARD T& operator[](usize i) noexcept { return _ptr[i]; }
        IO_NODISCARD const T& operator[](usize i) const noexcept { return _ptr[i]; }

        IO_NODISCARD T& front() noexcept { return _ptr[0]; }
        IO_NODISCARD T& back() noexcept { return _ptr[_len - 1]; }
        IO_NODISCARD const T& front() const noexcept { return _ptr[0]; }
        IO_NODISCARD const T& back() const noexcept { return _ptr[_len - 1]; }

        IO_NODISCARD bool reserve(usize new_cap) noexcept {
            if (new_cap <= _cap) return true;

            usize target = _cap ? (_cap * 2) : 8;
            if (target < new_cap) target = new_cap;

            const usize bytes = target * sizeof(T);
            u8* new_raw = static_cast<u8*>(alloc_aligned(bytes, alignof(T)));
            if (!new_raw) return false;

            T* new_ptr = reinterpret_cast<T*>(new_raw);

            for (usize i = 0; i < _len; ++i) {
                new (new_ptr + i) T(io::move(_ptr[i]));
                _ptr[i].~T();
            }

            _raw.reset(new_raw);
            _ptr = new_ptr;
            _cap = target;
            return true;
        }

        IO_NODISCARD bool resize(usize n) noexcept {
            if (n > _cap) {
                if (!reserve(n)) return false;
            }
            if (n > _len) {
                for (usize i=_len; i<n; ++i) new (_ptr+i) T{};
            }
            else if (n < _len) {
                for (usize i=_len; i>n; --i) _ptr[i-1].~T();
            }
            _len = n;
            return true;
        }

        IO_NODISCARD bool push_back(const T& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            new (_ptr + _len) T(v);
            ++_len;
            return true;
        }

        IO_NODISCARD bool push_back(T&& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            new (_ptr + _len) T(io::move(v));
            ++_len;
            return true;
        }

        void pop_back() noexcept {
            if (_len == 0) return;
            --_len;
            _ptr[_len].~T();
        }

        void clear() noexcept {
            destroy_range(_ptr, _len);
            _len = 0;
        }

        void set_size_unsafe(usize n) noexcept {
            // Caller promises elements [0..n) are constructed.
            if (n <= _cap) _len = n;
        }

        template<class CharT>
        IO_NODISCARD bool split(view<const CharT> src, CharT delim) noexcept {
            clear();
            usize start = 0;
            for (usize i = 0; i < src.size(); ++i) {
                if (src[i] == delim) {
                    if (!push_back(view<const CharT>{ src.data()+start, i-start })) return false;
                    start = i+1;
                }
            }
            return push_back(view<const CharT>{ src.data()+start, src.size()-start });
        }
    private:
        unique_bytes _raw{};
        T* _ptr{ nullptr };
        usize _len{ 0 };
        usize _cap{ 0 };

        static void destroy_range(T* p, usize n) noexcept {
            if (!p) return;
            for (usize i = n; i > 0; --i) p[i - 1].~T();
        }
    };
    // ------------------------- deque -----------------------------------
    // Ring-buffer deque. Allocates buffer via io::alloc_aligned().
    // Thread-safety: same as std::deque (external synchronization required).
    template<class T>
    struct deque {
        using value_type = T;

        IO_CONSTEXPR deque() noexcept = default;

        ~deque() noexcept {
            clear();
            _raw.reset(nullptr);
            _buf = nullptr;
            _cap = 0;
            _len = 0;
            _front = 0;
        }

        deque(const deque&) = delete;
        deque& operator=(const deque&) = delete;

        deque(deque&& o) noexcept {
            _raw = static_cast<unique_bytes&&>(o._raw);
            _buf = o._buf;      o._buf = nullptr;
            _cap = o._cap;      o._cap = 0;
            _len = o._len;      o._len = 0;
            _front = o._front;  o._front = 0;
        }

        deque& operator=(deque&& o) noexcept {
            if (this == &o) return *this;
            this->~deque();
            new (this) deque(static_cast<deque&&>(o));
            return *this;
        }

        // ---- size/capacity ----
        IO_NODISCARD usize size() const noexcept { return _len; }
        IO_NODISCARD usize capacity() const noexcept { return _cap; }
        IO_NODISCARD bool empty() const noexcept { return _len == 0; }

        // ---- element access ----
        IO_NODISCARD T& operator[](usize i) noexcept { return _buf[idx(i)]; }
        IO_NODISCARD const T& operator[](usize i) const noexcept { return _buf[idx(i)]; }

        IO_NODISCARD T& front() noexcept { return (*this)[0]; }
        IO_NODISCARD const T& front() const noexcept { return (*this)[0]; }

        IO_NODISCARD T& back() noexcept { return (*this)[_len - 1]; }
        IO_NODISCARD const T& back() const noexcept { return (*this)[_len - 1]; }

        // ---- modifiers ----
        IO_NODISCARD bool reserve(usize new_cap) noexcept {
            if (new_cap <= _cap) return true;

            usize target = _cap ? (_cap * 2) : 8;
            if (target < new_cap) target = new_cap;

            const usize bytes = target * sizeof(T);
            u8* new_raw = static_cast<u8*>(io::alloc_aligned(bytes, alignof(T)));
            if (!new_raw) return false;

            T* new_buf = reinterpret_cast<T*>(new_raw);

            // Move existing items in logical order into [0.._len).
            for (usize i = 0; i < _len; ++i) {
                new (new_buf + i) T(io::move((*this)[i]));
                (*this)[i].~T();
            }

            _raw.reset(new_raw);
            _buf = new_buf;
            _cap = target;
            _front = 0;
            return true;
        }

        IO_NODISCARD bool push_back(const T& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            new (_buf + idx(_len)) T(v);
            ++_len;
            return true;
        }

        IO_NODISCARD bool push_back(T&& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            new (_buf + idx(_len)) T(io::move(v));
            ++_len;
            return true;
        }

        IO_NODISCARD bool push_front(const T& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            dec_front();
            new (_buf + _front) T(v);
            ++_len;
            return true;
        }

        IO_NODISCARD bool push_front(T&& v) noexcept {
            if (_len == _cap && !reserve(_len + 1)) return false;
            dec_front();
            new (_buf + _front) T(io::move(v));
            ++_len;
            return true;
        }

        void pop_back() noexcept {
            if (_len == 0) return;
            const usize bi = idx(_len - 1);
            _buf[bi].~T();
            --_len;
            if (_len == 0) _front = 0;
        }

        void pop_front() noexcept {
            if (_len == 0) return;
            _buf[_front].~T();
            _front = (_front + 1) % _cap; // _cap>0 because _len>0
            --_len;
            if (_len == 0) _front = 0;
        }

        void clear() noexcept {
            if (!_buf || _len == 0) { _len = 0; _front = 0; return; }
            for (usize i = 0; i < _len; ++i) {
                (*this)[i].~T();
            }
            _len = 0;
            _front = 0;
        }

    private:
        unique_bytes _raw{};
        T* _buf{ nullptr };
        usize _cap{ 0 };
        usize _len{ 0 };
        usize _front{ 0 };

        IO_NODISCARD usize idx(usize i) const noexcept {
            // Valid only when _cap > 0.
            return (_front + i) % _cap;
        }

        void dec_front() noexcept {
            // Called only when _cap > 0.
            _front = (_front == 0) ? (_cap - 1) : (_front - 1);
        }
    }; // struct deque
    // ------------------------- basic_string -----------------------------
    template<class CharT>
    struct basic_string {
        using view_t = view<const CharT>;
        vector<CharT> v;

        IO_CONSTEXPR basic_string() noexcept {
            // Keep '\0' invariant: always one terminator allocated and constructed.
            (void)v.push_back(CharT(0));
            v.set_size_unsafe(1);
        }
        basic_string(const basic_string&) = delete;
        basic_string& operator=(const basic_string&) = delete;
        basic_string(basic_string&& o) noexcept : v(static_cast<vector<CharT>&&>(o.v)) {}
        basic_string& operator=(basic_string&& o) noexcept { v = static_cast<vector<CharT>&&>(o.v); return *this; }
        explicit basic_string(view<const CharT> s) noexcept { init_empty(); (void)append(s); }
        explicit basic_string(const CharT* s) noexcept { init_empty(); if (s) (void)append_cstr(s); }

        IO_NODISCARD usize size() const noexcept {
            const usize n = v.size();
            return n ? n-1 : 0; // don't count '\0'
        }
        IO_NODISCARD bool empty() const noexcept { return size() == 0; }

        IO_NODISCARD const CharT* c_str() const noexcept { return v.data() ? v.data() : zlit(); }
        IO_NODISCARD CharT* data() noexcept { return v.data(); }
        IO_NODISCARD const CharT* data() const noexcept { return c_str(); }

        IO_NODISCARD CharT& operator[](usize i) noexcept { return v[i]; }
        IO_NODISCARD const CharT& operator[](usize i) const noexcept { return v[i]; }

        IO_NODISCARD view<const CharT> as_view() const noexcept { return view<const CharT>(v.data(), size()); }
        IO_NODISCARD view<CharT> as_mut_view() noexcept { return view<CharT>(v.data(), size()); }
        IO_NODISCARD bool reserve(usize n) noexcept { return v.reserve(n + 1); }

        void clear() noexcept {
            v.clear();
            (void)v.push_back(CharT(0));
            v.set_size_unsafe(1);
        }
        IO_NODISCARD bool resize(usize n, CharT fill = CharT(0)) noexcept {
            const usize old = size();
            if (!ensure_terminator()) return false;

            if (n < old) {
                if (!v.resize(n + 1)) return false;
                v[n] = CharT(0);
                return true;
            }

            if (n > old) {
                if (!v.resize(n + 1)) return false;
                for (usize i = old; i < n; ++i) v[i] = fill;
                v[n] = CharT(0);
            }
            return true;
        }
        IO_NODISCARD bool push_back(CharT ch) noexcept {
            const usize n = size();
            if (!ensure_terminator()) return false;
            if (!v.resize(n + 2)) return false;
            v[n] = ch;
            v[n + 1] = CharT(0);
            return true;
        }
        IO_NODISCARD bool append(view<const CharT> s) noexcept {
            if (s.size() == 0) return true;
            const usize n = size();
            if (!ensure_terminator()) return false;

            if (!v.resize(n+1 + s.size())) return false;
            for (usize i=0; i < s.size(); ++i) v[n+i] = s[i];
            v[n + s.size()] = CharT(0);
            return true;
        }
        IO_NODISCARD bool append(const CharT* s) noexcept {
            if (!s) return true;
            return append_cstr(s);
        }
        IO_NODISCARD bool append(const basic_string<CharT>& s) noexcept { return append(s.as_view()); }

        IO_NODISCARD bool operator+=(view<const CharT> s) noexcept { return append(s); }
        IO_NODISCARD bool operator+=(const CharT* s) noexcept { return append(s); }
        IO_NODISCARD bool operator+=(CharT ch) noexcept { return push_back(ch); }

        friend bool operator==(const basic_string& a, view<const CharT> b) noexcept {
            if (a.size() != b.size()) return false;
            for (usize i = 0; i < b.size(); ++i) if (a[i] != b[i]) return false;
            return true;
        }

        IO_NODISCARD bool split(CharT delim, vector<view_t>& out_parts) const noexcept {
            return out_parts.split(as_view(), delim);
        }

        IO_NODISCARD static bool join(view<view_t> parts, view_t delim, basic_string& out) noexcept {
            out.clear();

            usize total = 0;
            for (usize i = 0; i < parts.size(); ++i) {
                total += parts[i].size();
                if (i + 1 < parts.size()) total += delim.size();
            }
            if (!out.reserve(total)) return false;

            for (usize i = 0; i < parts.size(); ++i) {
                if (i) (void)out.append(delim);
                (void)out.append(parts[i]);
            }
            return true;
        }
        static bool join(view<view_t> parts, CharT delim, basic_string& out) noexcept { return join(parts, view_t{ &delim, 1 }, out); }
        static bool join(const vector<view_t>& parts, view_t delim, basic_string& out) noexcept { return join(parts.as_view(), delim, out); }
        static bool join(const vector<view_t>& parts, CharT delim, basic_string& out) noexcept { return join(parts.as_view(), delim, out); }
        static bool join(view<view_t> parts, basic_string& out) noexcept { return join(parts, view_t{}, out); }
        static bool join(const vector<view_t>& parts, basic_string& out) noexcept { return join(parts.as_view(), out); }

    private:
        static const CharT* zlit() noexcept {
            static const CharT z[1] = { CharT(0) };
            return z;
        }

        void init_empty() noexcept {
            v.clear();
            (void)v.push_back(CharT(0));
            v.set_size_unsafe(1);
        }

        IO_NODISCARD bool ensure_terminator() noexcept {
            if (v.size() == 0) return v.push_back(CharT(0));
            if (v[v.size() - 1] != CharT(0)) return v.push_back(CharT(0));
            return true;
        }

        IO_NODISCARD bool append_cstr(const CharT* s) noexcept {
            usize n = 0;
            while (s[n] != CharT(0)) ++n;
            return append(view<const CharT>(s, n));
        }
    };

    using string = basic_string<char>;
    using wstring = basic_string<wchar_t>;
    // ------------------------- list ------------------------------------
    // Allocates nodes via io::alloc_aligned().
    // Thread-safety: same as std::list (external synchronization required).
    template<class T>
    struct list {
        using value_type = T;

        struct node {
            node* prev;
            node* next;
            T value;

            node(const T& v) : prev(nullptr), next(nullptr), value(v) {}
            node(T&& v) : prev(nullptr), next(nullptr), value(io::move(v)) {}
        };

        struct iterator {
            node* p{ nullptr };

            T& operator*() const noexcept { return p->value; }
            T* operator->() const noexcept { return &p->value; }

            iterator& operator++() noexcept { p = p ? p->next : nullptr; return *this; }
            iterator operator++(int) noexcept { iterator t = *this; ++(*this); return t; }

            iterator& operator--() noexcept { p = p ? p->prev : nullptr; return *this; }

            friend bool operator==(iterator a, iterator b) noexcept { return a.p == b.p; }
            friend bool operator!=(iterator a, iterator b) noexcept { return a.p != b.p; }
        };

        struct const_iterator {
            const node* p{ nullptr };

            const T& operator*() const noexcept { return p->value; }
            const T* operator->() const noexcept { return &p->value; }

            const_iterator& operator++() noexcept { p = p ? p->next : nullptr; return *this; }
            const_iterator operator++(int) noexcept { const_iterator t = *this; ++(*this); return t; }

            friend bool operator==(const_iterator a, const_iterator b) noexcept { return a.p == b.p; }
            friend bool operator!=(const_iterator a, const_iterator b) noexcept { return a.p != b.p; }
        };

        IO_CONSTEXPR list() noexcept = default;
        ~list() noexcept { clear(); }

        list(const list&) = delete;
        list& operator=(const list&) = delete;

        list(list&& o) noexcept {
            _head = o._head;
            _tail = o._tail;
            _len = o._len;
            o._head = o._tail = nullptr;
            o._len = 0;
        }

        list& operator=(list&& o) noexcept {
            if (this == &o) return *this;
            this->~list();
            new (this) list(static_cast<list&&>(o));
            return *this;
        }

        IO_NODISCARD usize size() const noexcept { return _len; }
        IO_NODISCARD bool empty() const noexcept { return _len == 0; }

        iterator begin() noexcept { return iterator{ _head }; }
        iterator end() noexcept { return iterator{ nullptr }; }
        const_iterator begin() const noexcept { return const_iterator{ _head }; }
        const_iterator end() const noexcept { return const_iterator{ nullptr }; }

        T& front() noexcept { return _head->value; }
        T& back() noexcept { return _tail->value; }
        const T& front() const noexcept { return _head->value; }
        const T& back() const noexcept { return _tail->value; }

        IO_NODISCARD bool push_back(const T& v) noexcept { return link_back(make_node(v)); }
        IO_NODISCARD bool push_back(T&& v) noexcept { return link_back(make_node(io::move(v))); }

        IO_NODISCARD bool push_front(const T& v) noexcept { return link_front(make_node(v)); }
        IO_NODISCARD bool push_front(T&& v) noexcept { return link_front(make_node(io::move(v))); }

        void pop_back() noexcept {
            if (!_tail) return;

            node* n = _tail;
            _tail = n->prev;

            if (_tail) _tail->next = nullptr;
            else _head = nullptr;

            destroy_node(n);
            --_len;
        }

        void pop_front() noexcept {
            if (!_head) return;

            node* n = _head;
            _head = n->next;

            if (_head) _head->prev = nullptr;
            else _tail = nullptr;

            destroy_node(n);
            --_len;
        }

        iterator erase(iterator it) noexcept {
            node* n = it.p;
            if (!n) return end();

            node* nx = n->next;

            if (n->prev) n->prev->next = n->next;
            else _head = n->next;

            if (n->next) n->next->prev = n->prev;
            else _tail = n->prev;

            destroy_node(n);
            --_len;
            return iterator{ nx };
        }

        void clear() noexcept {
            node* cur = _head;
            while (cur) {
                node* nx = cur->next;
                destroy_node(cur);
                cur = nx;
            }
            _head = _tail = nullptr;
            _len = 0;
        }

    private:
        node* _head{ nullptr };
        node* _tail{ nullptr };
        usize _len{ 0 };

        template<class U>
        node* make_node(U&& v) noexcept {
            void* mem = io::alloc_aligned(sizeof(node), alignof(node));
            if (!mem) return nullptr;
            return new (mem) node(static_cast<U&&>(v));
        }

        static void destroy_node(node* n) noexcept {
            if (!n) return;
            n->~node();
            io::free_aligned(n);
        }

        bool link_back(node* n) noexcept {
            if (!n) return false;
            n->prev = _tail;
            n->next = nullptr;
            if (_tail) _tail->next = n;
            else _head = n;
            _tail = n;
            ++_len;
            return true;
        }

        bool link_front(node* n) noexcept {
            if (!n) return false;
            n->prev = nullptr;
            n->next = _head;
            if (_head) _head->prev = n;
            else _tail = n;
            _head = n;
            ++_len;
            return true;
        }
    };

    // ========================================================================
    //                              Out
    // ========================================================================
    // ----------------------------------------------------------------------------
// Low-level native terminal write/read
// ----------------------------------------------------------------------------
#if defined(IO_IMPLEMENTATION)

#if defined(IO_TERMINAL) && defined(_WIN32)
    static inline usize utf8_safe_chunk_len(const char* s, usize rem, usize max_bytes) noexcept {
        if (!s || rem == 0 || max_bytes == 0) return 0;

        usize n = rem < max_bytes ? rem : max_bytes;
        if (n == rem) return n;

        auto is_cont = [](unsigned char c) -> bool { return (c & 0xC0u) == 0x80u; };

        usize end = n;
        while (end > 0 && is_cont(static_cast<unsigned char>(s[end - 1]))) --end;
        if (end == n) return n;
        if (end == 0) return 1;

        unsigned char lead = static_cast<unsigned char>(s[end - 1]);

        usize seq_len = 1;
        if ((lead & 0x80u) == 0x00u) seq_len = 1;
        else if ((lead & 0xE0u) == 0xC0u) seq_len = 2;
        else if ((lead & 0xF0u) == 0xE0u) seq_len = 3;
        else if ((lead & 0xF8u) == 0xF0u) seq_len = 4;
        else return end > 1 ? (end - 1) : 1;

        usize seq_start = end - 1;
        usize seq_end = seq_start + seq_len;
        if (seq_end <= n) return n;
        return seq_start > 0 ? seq_start : 1;
    }

    static inline void native_terminal_write(const char* msg, usize len) noexcept {
        if (!msg || len == 0) return;

        HANDLE hterminal = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (hterminal == INVALID_HANDLE_VALUE || hterminal == nullptr) return;

        wchar_t wbuf[256];
        usize pos = 0;

        while (pos < len) {
            const usize max_chunk = 200;
            usize rem = len - pos;
            usize chunk = utf8_safe_chunk_len(msg + pos, rem, max_chunk);
            if (chunk == 0) break;

            int wlen = ::MultiByteToWideChar(
                CP_UTF8, 0,
                msg + pos, (int)chunk,
                wbuf, (int)(sizeof(wbuf) / sizeof(wbuf[0]))
            );

            if (wlen <= 0) {
                DWORD written = 0;
                wchar_t q = L'?';
                ::WriteConsoleW(hterminal, &q, 1, &written, nullptr);
                pos += chunk;
                continue;
            }

            DWORD written = 0;
            ::WriteConsoleW(hterminal, wbuf, (DWORD)wlen, &written, nullptr);
            pos += chunk;
        }
    }

    static inline usize native_terminal_read(char* dst, usize cap) noexcept {
        if (!dst || cap == 0) return 0;

        HANDLE h = ::GetStdHandle(STD_INPUT_HANDLE);
        if (h == INVALID_HANDLE_VALUE || h == nullptr) {
            dst[0] = 0;
            return 0;
        }

        wchar_t wbuf[256];
        DWORD read = 0;
        DWORD max_read = (DWORD)((sizeof(wbuf) / sizeof(wbuf[0])) - 1);

        if (!::ReadConsoleW(h, wbuf, max_read, &read, nullptr)) {
            dst[0] = 0;
            return 0;
        }

        while (read > 0 && (wbuf[read - 1] == L'\n' || wbuf[read - 1] == L'\r')) --read;

        int bytes = ::WideCharToMultiByte(
            CP_UTF8, 0,
            wbuf, (int)read,
            dst, (int)(cap - 1),
            nullptr, nullptr
        );

        if (bytes < 0) bytes = 0;
        dst[(usize)bytes] = 0;
        return (usize)bytes;
    }
#endif

#if defined(IO_TERMINAL) && defined(__linux__)
    static inline void native_terminal_write(const char* msg, usize len) noexcept {
        if (!msg || len == 0) return;

        usize pos = 0;
        while (pos < len) {
            const unsigned long n = (unsigned long)(len - pos);
            long r = native::linux_sys_write(1, msg + pos, n);
            if (r > 0) { pos += (usize)r; continue; }
            if (r == 0) break;
            const long err = -r;
            if (err == 4) continue; // EINTR
            break;
        }
    }

    static inline usize utf8_encode_u32(char out[4], u32 cp) noexcept {
        if (cp <= 0x7Fu) {
            out[0] = (char)cp;
            return 1;
        }
        if (cp <= 0x7FFu) {
            out[0] = (char)(0xC0u | (cp >> 6));
            out[1] = (char)(0x80u | (cp & 0x3Fu));
            return 2;
        }
        if (cp <= 0xFFFFu) {
            out[0] = (char)(0xE0u | (cp >> 12));
            out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
            out[2] = (char)(0x80u | (cp & 0x3Fu));
            return 3;
        }
        if (cp <= 0x10FFFFu) {
            out[0] = (char)(0xF0u | (cp >> 18));
            out[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
            out[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
            out[3] = (char)(0x80u | (cp & 0x3Fu));
            return 4;
        }
        out[0] = '?';
        return 1;
    }

    static inline usize native_terminal_read(char* dst, usize cap) noexcept {
        if (!dst || cap == 0) return 0;
        if (cap == 1) { dst[0] = 0; return 0; }

        const unsigned long want = (unsigned long)(cap - 1);

        for (;;) {
            long r = native::linux_sys_read(0, dst, want);
            if (r > 0) {
                usize n = (usize)r;
                while (n > 0) {
                    char c = dst[n - 1];
                    if (c == '\n' || c == '\r') { --n; continue; }
                    break;
                }
                dst[n] = 0;
                return n;
            }
            if (r == 0) { dst[0] = 0; return 0; }
            const long err = -r;
            if (err == 4) continue; // EINTR
            dst[0] = 0;
            return 0;
        }
    }
#endif

#if !defined(IO_TERMINAL)
    static inline void native_terminal_write(const char*, usize) noexcept {}
    static inline usize native_terminal_read(char* dst, usize cap) noexcept {
        if (dst && cap) dst[0] = 0;
        return 0;
    }
#endif

#endif // IO_IMPLEMENTATION

    // ----------------------------------------------------------------------------
    // Sink / Source types
    // ----------------------------------------------------------------------------

    struct TerminalSink {
        spin_mutex m;

        bool write(const char* s, usize n) noexcept {
#if !defined(IO_TERMINAL)
            (void)s; (void)n;
            return true;
#else
            if (!s || n == 0) return true;
            m.lock();
#ifdef IO_IMPLEMENTATION
            native_terminal_write(s, n);
#endif
            m.unlock();
            return true;
#endif
        }

        void flush() noexcept {}
        void reset() noexcept {}
        char_view view() const noexcept { return {}; }
    };

    struct TerminalSource {
        spin_mutex m;

        usize read(char* dst, usize cap) noexcept {
#if !defined(IO_TERMINAL)
            if (dst && cap) dst[0] = 0;
            return 0;
#else
            if (!dst || cap == 0) return 0;
            m.lock();
#   ifdef IO_IMPLEMENTATION
            usize n = native_terminal_read(dst, cap);
            m.unlock();
            return n;
#   else
            return 0;
#   endif
#endif
        }
    };

    template<usize N>
    struct BufferSink {
        char buf[N]{};
        usize count = 0;

        bool write(const char* s, usize n) noexcept {
            if (!s) return false;
            if (N == 0) return false;

            usize avail = (count < N) ? (N - count) : 0;
            if (avail == 0) return false;

            usize max_copy = avail - 1; // reserve for '\0'
            usize take = (n < max_copy) ? n : max_copy;

            for (usize i = 0; i < take; ++i) buf[count++] = s[i];
            buf[count] = '\0';
            return take == n;
        }

        void flush() noexcept {}

        void reset() noexcept {
            count = 0;
            if (N > 0) buf[0] = '\0';
        }

        char_view view() const noexcept {
            return char_view{ buf, count };
        }
    };

    template<usize N>
    struct ThreadSafeBufferSink {
        spin_mutex m;
        char buf[N]{};
        usize count = 0;

        bool write(const char* s, usize n) noexcept {
            if (!s) return false;
            if (N == 0) return false;

            m.lock();

            usize avail = (count < N) ? (N - count) : 0;
            if (avail == 0) {
                m.unlock();
                return false;
            }

            usize max_copy = avail - 1;
            usize take = (n < max_copy) ? n : max_copy;

            for (usize i = 0; i < take; ++i) buf[count++] = s[i];
            buf[count] = '\0';

            m.unlock();
            return take == n;
        }

        void flush() noexcept {}

        void reset() noexcept {
            m.lock();
            count = 0;
            if (N > 0) buf[0] = '\0';
            m.unlock();
        }

        char_view view() const noexcept {
            return char_view{ buf, count };
        }
    };

    // ----------------------------------------------------------------------------
    // OutT
    // ----------------------------------------------------------------------------
    template<class SinkT>
    struct OutT {
        using Self = OutT<SinkT>;
        SinkT* sink = nullptr;

        IO_CONSTEXPR OutT() noexcept = default;
        IO_CONSTEXPR explicit OutT(SinkT& s) noexcept : sink(&s) {}

        inline void reset() noexcept {
            if (sink) sink->reset();
        }

        inline void write(const char* msg, usize len) noexcept {
            if (!sink || !msg || len == 0) return;
            (void)sink->write(msg, len);
        }

        inline void flush() noexcept {
            if (sink) sink->flush();
        }

        inline char_view view() const noexcept {
            if (!sink) return {};
            return sink->view();
        }

    protected:
        inline void put(char c) const noexcept {
            if (!sink) return;
            (void)sink->write(&c, 1);
        }

        inline void write_str(const char* str, usize count) const noexcept {
            if (!sink || !str || count == 0) return;
            (void)sink->write(str, count);
        }

        inline void write_str(const char* str) const noexcept {
            if (!sink || !str) return;
            write_str(str, ::io::len(str));
        }

        inline void write_unsigned(u64 v) const noexcept {
            if (!sink) return;

            char tmp[32]{};
            int i = 0;
            do {
                u32 rem = 0;
                v = div_u64_u32(v, 10u, &rem);
                tmp[i++] = (char)('0' + rem);
            } while (v);

            while (--i >= 0) put(tmp[i]);
        }

        inline void write_signed(i64 v) const noexcept {
            if (v < 0) {
                put('-');
                const u64 uv = (u64)(-(v + 1)) + 1u; // safe for INT64_MIN
                write_unsigned(uv);
            }
            else {
                write_unsigned((u64)v);
            }
        }

        static inline unsigned pow10(int n) noexcept {
            unsigned r = 1;
            while (n-- > 0) r *= 10u;
            return r;
        }

        inline void write_float(double x, int precision = 6) const noexcept {
#if !defined(IO_HAS_STD) && IO_ARCH_X86_32
            if (precision < 0) precision = 0;
            if (precision > 9) precision = 9;
        
            if (io_double_is_nan(x)) {
                write_str("nan");
                return;
            }
            if (io_double_is_inf(x)) {
                if (x < 0) put('-');
                write_str("inf");
                return;
            }
        
            if (x < 0.0) {
                put('-');
                x = -x;
            }
        
            if (x == 0.0) {
                put('0');
                if (precision > 0) {
                    put('.');
                    for (int i = 0; i < precision; ++i) put('0');
                }
                return;
            }
        
            // Use scientific notation for very large / very small numbers.
            // This avoids huge fixed strings and keeps implementation simple.
            if (x >= 1000000000.0 || x < 0.000001) {
                int exp10 = 0;
        
                // normalize x into [1, 10)
                static const double p10_pos[] = {
                    1e256, 1e128, 1e64, 1e32, 1e16, 1e8, 1e4, 1e2, 1e1
                };
                static const int p10_pos_e[] = {
                    256, 128, 64, 32, 16, 8, 4, 2, 1
                };
        
                static const double p10_neg[] = {
                    1e-256, 1e-128, 1e-64, 1e-32, 1e-16, 1e-8, 1e-4, 1e-2, 1e-1
                };
                static const int p10_neg_e[] = {
                    256, 128, 64, 32, 16, 8, 4, 2, 1
                };
        
                if (x >= 10.0) {
                    for (int i = 0; i < (int)(sizeof(p10_pos) / sizeof(p10_pos[0])); ++i) {
                        while (x >= p10_pos[i]) {
                            x /= p10_pos[i];
                            exp10 += p10_pos_e[i];
                        }
                    }
                } else if (x < 1.0) {
                    for (int i = 0; i < (int)(sizeof(p10_neg) / sizeof(p10_neg[0])); ++i) {
                        while (x < p10_neg[i]) {
                            x /= p10_neg[i];
                            exp10 -= p10_neg_e[i];
                        }
                    }
                    while (x < 1.0) {
                        x *= 10.0;
                        --exp10;
                    }
                }
        
                // one digit before dot
                io::u32 d = io_digit_to_u32_from_double_pos(x);
                if (d > 9) d = 9;
                put((char)('0' + d));
                x -= io_digit_to_double(d);
        
                if (precision > 0) {
                    put('.');
                    for (int i = 0; i < precision; ++i) {
                        x *= 10.0;
                        io::u32 fd = io_digit_to_u32_from_double_pos(x);
                        if (fd > 9) fd = 9;
                        put((char)('0' + fd));
                        x -= io_digit_to_double(fd);
                        if (x < 0.0) x = 0.0;
                    }
                }
        
                put('e');
                if (exp10 >= 0) put('+');
                else {
                    put('-');
                    exp10 = -exp10;
                }
                write_unsigned((io::u32)exp10);
                return;
            }
        
            // Fixed notation for ordinary values: 0.000001 <= x < 1e9
            {
                static const double p10[] = {
                    1e8, 1e7, 1e6, 1e5, 1e4, 1e3, 1e2, 1e1, 1e0
                };
        
                bool started = false;
        
                // integer part
                for (int i = 0; i < (int)(sizeof(p10) / sizeof(p10[0])); ++i) {
                    const double base = p10[i];
                    io::u32 digit = 0;
        
                    if (x >= base) {
                        digit = io_digit_to_u32_from_double_pos(x / base);
                        if (digit > 9) digit = 9;
                        started = true;
                    }
        
                    if (started) {
                        put((char)('0' + digit));
                        x -= io_digit_to_double(digit) * base;
                        if (x < 0.0) x = 0.0;
                    }
                }
        
                if (!started) put('0');
        
                if (precision > 0) {
                    put('.');
                    for (int i = 0; i < precision; ++i) {
                        x *= 10.0;
                        io::u32 digit = io_digit_to_u32_from_double_pos(x);
                        if (digit > 9) digit = 9;
                        put((char)('0' + digit));
                        x -= io_digit_to_double(digit);
                        if (x < 0.0) x = 0.0;
                    }
                }
            }
        
#else // not "32 bit no-std"
            if (x < 0) { put('-'); x = -x; }
        
            u64 ip = (u64)x;
            double fp = x - (double)ip;
        
            write_unsigned(ip);
            put('.');
        
            fp *= (double)pow10(precision);
            write_unsigned((u64)(fp + 0.5));
#endif
        }

    public:
        inline const Self& operator<<(double v) const noexcept { write_float(v); return *this; }
        inline const Self& operator<<(bool b) const noexcept { write_str(b ? "true" : "false"); return *this; }
        inline const Self& operator<<(const char* s) const noexcept { write_str(s); return *this; }
        inline const Self& operator<<(const unsigned char* s) const noexcept { return *this << reinterpret_cast<const char*>(s); }
        inline const Self& operator<<(char c) const noexcept { put(c); return *this; }
        inline const Self& operator<<(i8 v)  const noexcept { return *this << (i64)v; }
        inline const Self& operator<<(i16 v) const noexcept { return *this << (i64)v; }
        inline const Self& operator<<(i32 v) const noexcept { return *this << (i64)v; }
        inline const Self& operator<<(long v) const noexcept { return *this << (i64)v; }
        inline const Self& operator<<(i64 v) const noexcept { write_signed(v); return *this; }
        inline const Self& operator<<(u8 v)  const noexcept { return *this << (u64)v; }
        inline const Self& operator<<(u16 v) const noexcept { return *this << (u64)v; }
        inline const Self& operator<<(u32 v) const noexcept { return *this << (u64)v; }
        inline const Self& operator<<(unsigned long v) const noexcept { return *this << (u64)v; }
        inline const Self& operator<<(u64 v) const noexcept { write_unsigned(v); return *this; }

        inline const Self& operator<<(char_view v) const noexcept {
            write_str(v.data(), v.size());
            return *this;
        }

        inline const Self& operator<<(const string& v) const noexcept { return *this << v.as_view(); }

        inline const Self& operator<<(const wstring& w) const noexcept {
#if defined(_WIN32)
            if (!sink || w.size() == 0) return *this;
            TerminalSink* ts = reinterpret_cast<TerminalSink*>(sink);
            if (ts) {
                ts->m.lock();
                HANDLE hterminal = ::GetStdHandle(STD_OUTPUT_HANDLE);
                if (hterminal != INVALID_HANDLE_VALUE && hterminal != nullptr) {
                    DWORD written = 0;
                    ::WriteConsoleW(hterminal, w.data(), (DWORD)w.size(), &written, nullptr);
                }
                ts->m.unlock();
            }
            return *this;
#elif defined(__linux__)
            if (w.size() == 0) return *this;

            char buf[256];
            usize bi = 0;

            for (usize i = 0; i < w.size(); ++i) {
                u32 cp = (u32)w.data()[i];
                char tmp[4];
                usize n = utf8_encode_u32(tmp, cp);

                if (bi + n > sizeof(buf)) {
                    write(buf, bi);
                    bi = 0;
                }
                for (usize k = 0; k < n; ++k) buf[bi++] = tmp[k];
            }
            if (bi) write(buf, bi);
            return *this;
#else
#   error "Not implemented"
#endif
        }

        inline const Self& operator<<(const Self& (*manip)(const Self&)) const noexcept {
            return manip(*this);
        }

        struct StrPrinter {
            const u8* data;
            usize size;

            inline const Self& operator()(const Self& o) const noexcept {
                for (usize i = 0; i < size; ++i)
                    o.put((char)data[i]);
                return o;
            }
        };

        struct HexPrinter {
            const void* data;
            usize size;

            inline const Self& operator()(const Self& o) const noexcept {
                IO_CONSTEXPR_VAR char digits[] = "0123456789abcdef";
                const u8* p = static_cast<const u8*>(data);
                for (usize i = 0; i < size; ++i) {
                    o.put(digits[(p[i] >> 4) & 0x0F]);
                    o.put(digits[p[i] & 0x0F]);
                }
                return o;
            }
        };

        static inline StrPrinter str(const u8* data, usize size) noexcept { return { data, size }; }
        static inline HexPrinter hex(const void* data, usize size) noexcept { return { data, size }; }

        inline const Self& operator<<(HexPrinter p) const noexcept { return p(*this); }
        inline const Self& operator<<(StrPrinter p) const noexcept { return p(*this); }
    };

    // ----------------------------------------------------------------------------
    // InT
    // ----------------------------------------------------------------------------
    template<class SourceT>
    struct InT {
        using Self = InT<SourceT>;
        SourceT* source = nullptr;

        constexpr InT() noexcept = default;
        constexpr explicit InT(SourceT& s) noexcept : source(&s) {}

        inline usize read(view<char> v) const noexcept {
            if (!source || !v.data() || v.size() == 0) return 0;
            usize n = source->read(v.data(), v.size());
            if (n < v.size()) v[n] = 0;
            return n;
        }
    };

    template<class SourceT>
    inline InT<SourceT>& operator>>(InT<SourceT>& in_obj, view<char> v) noexcept {
        if (!v.data() || v.size() == 0) return in_obj;
        usize n = in_obj.read(v);
        if (n < v.size()) v[n] = 0;
        return in_obj;
    }

    // ----------------------------------------------------------------------------
    // Convenient aliases
    // ----------------------------------------------------------------------------
    using Out = OutT<TerminalSink>;
    using In = InT<TerminalSource>;

    template<usize N>
    using SinkSize = BufferSink<N>;

    template<class SinkT>
    using SinkOut = OutT<SinkT>;

    template<usize N>
    struct StackOut : OutT<BufferSink<N>> {
        IO_NODISCARD IO_CONSTEXPR usize value_type() const noexcept { return N; }
        using Base = OutT<BufferSink<N>>;
        BufferSink<N> storage{};

        StackOut() noexcept : Base(storage) {}
    };

    template<usize N>
    struct StackIn {
        char buf[N]{};
        usize count = 0;
    };

    // ----------------------------------------------------------------------------
    // Globals
    // ----------------------------------------------------------------------------
    namespace global {
        static TerminalSink   g_terminal_sink{};
        static TerminalSource g_terminal_source{};
    }

    static const Out out{ global::g_terminal_sink };
    static const In  in{ global::g_terminal_source };

    enum class SeekWhence : u8 {
        Begin,
        Current,
        End
    };
    
    enum class OpenMode : u8 {
        None     = 0,
        Read     = 1u<<0,
        Write    = 1u<<1,
        Append   = 1u<<2, // write at the end
        Truncate = 1u<<3, // truncate on open
        Create   = 1u<<4, // Create if not exists
        Binary   = 1u<<5, // for Win/Unix there's almost no difference
        Text     = 1u<<6  // Optionally (immutable state)
    };

    IO_NODISCARD IO_CONSTEXPR OpenMode operator|(OpenMode a, OpenMode b) noexcept { return static_cast<OpenMode>(static_cast<u8>(a) | static_cast<u8>(b)); }
    IO_NODISCARD IO_CONSTEXPR OpenMode operator&(OpenMode a, OpenMode b) noexcept { return static_cast<OpenMode>(static_cast<u8>(a) & static_cast<u8>(b)); }
    IO_CONSTEXPR OpenMode& operator|=(OpenMode& a, OpenMode b) noexcept { a=a|b; return a; }
    IO_CONSTEXPR OpenMode& operator&=(OpenMode& a, OpenMode b) noexcept { a=a&b; return a; }
    IO_NODISCARD IO_CONSTEXPR bool has(OpenMode m, OpenMode f) noexcept { return (static_cast<u8>(m) & static_cast<u8>(f)) != 0; }
    
    namespace native {
        struct file_handle; // opaque; in `IO_IMPLEMENTATION` block
    
        IO_NODISCARD bool  open_file(file_handle*& out, char_view utf8_path, OpenMode mode) noexcept;
        void               close_file(file_handle* h) noexcept;
        IO_NODISCARD usize read_file(file_handle* h, void* dst, usize bytes) noexcept;   // 0 => eof or error
        IO_NODISCARD usize write_file(file_handle* h, const void* src, usize bytes) noexcept;
        IO_NODISCARD bool  flush_file(file_handle* h) noexcept;
        IO_NODISCARD bool  seek_file(file_handle* h, i64 offset, SeekWhence whence) noexcept;
        IO_NODISCARD u64   tell_file(file_handle* h) noexcept;
        IO_NODISCARD u64   size_file(file_handle* h) noexcept;
        IO_NODISCARD bool  is_eof(file_handle* h) noexcept;
        IO_NODISCARD bool  has_error(file_handle* h) noexcept;
        void               clear_error(file_handle* h) noexcept;
    } // namespace native

    // ========================================================================
    //                   native  F I L E
    // ========================================================================
#ifdef IO_IMPLEMENTATION
namespace native {

#if defined(_WIN32)
    // -------- UTF-8 <-> UTF-16 helpers (RAII via io::wstring) --------
    static inline bool utf8_to_wide(io::char_view utf8, io::wstring& out) noexcept {
        out.clear();
        if (!utf8) return false;
        // required wchar count WITHOUT null terminator
        const int need = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
        if (need <= 0) return false;
        if (!out.resize(static_cast<io::usize>(need))) return false;
        const int wrote = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), out.data(), need);
        if (wrote != need) return false;
        out.data()[need] = L'\0';
        return true;
    }
    static inline bool wide_to_utf8(const wchar_t* w, io::string& out) noexcept {
        out.clear();
        if (!w) return false;
        // required bytes including null terminator
        const int need = ::WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        if (need <= 0) return false;
        if (!out.resize(static_cast<io::usize>(need-1))) return false; // `need` includes '\0'
        const int wrote = ::WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), need, nullptr, nullptr);
        return wrote == need;
    }

    struct file_handle {
        HANDLE h{ INVALID_HANDLE_VALUE };
        bool eof{ false };
        bool err{ false };
        OpenMode mode{ OpenMode::None };
    };
    IO_CONSTEXPR static DWORD access_from_mode(OpenMode m) noexcept {
        DWORD acc = 0;
        if (has(m, OpenMode::Read))  acc |= GENERIC_READ;
        if (has(m, OpenMode::Write) || has(m, OpenMode::Append)) acc |= GENERIC_WRITE;
        return acc;
    }
    IO_CONSTEXPR static DWORD disposition_from_mode(OpenMode m) noexcept {
        const bool rd = has(m, OpenMode::Read);
        const bool wr = has(m, OpenMode::Write) || has(m, OpenMode::Append);
        const bool tr = has(m, OpenMode::Truncate);
        const bool cr = has(m, OpenMode::Create);
        if (wr) {
            if (tr) return CREATE_ALWAYS;          // truncate (and create)
            if (cr) return OPEN_ALWAYS;            // create if missing
            return OPEN_EXISTING;                  // must exist
        } // else: read-only
        (void)rd; 
        return OPEN_EXISTING;
    }
    IO_NODISCARD inline bool open_file(file_handle*& out, char_view utf8_path, OpenMode mode) noexcept {
        out = nullptr;
        if (!utf8_path.data() || utf8_path.empty()) return false;

        wstring wpath;
        if (!utf8_to_wide(utf8_path, wpath)) return false;

        file_handle* fh = heap_new<file_handle>();
        if (!fh) return false;

        const DWORD access = access_from_mode(mode);
        const DWORD disp = disposition_from_mode(mode);
        IO_CONSTEXPR_VAR DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        IO_CONSTEXPR_VAR DWORD flags = FILE_ATTRIBUTE_NORMAL;
        HANDLE h = ::CreateFileW(
                       /* lpFileName */ wpath.c_str(),
                  /* dwDesiredAccess */ access,
                      /* dwShareMode */ share,
             /* dwSecurityAttributes */ nullptr,
            /* dwCreationDisposition */ disp,
             /* dwFlagsAndAttributes */ flags,
                    /* hTemplateFile */ nullptr
        );
        if (h == INVALID_HANDLE_VALUE) { heap_delete(fh); return false; }
        fh->h = h;
        fh->mode = mode;
        fh->eof = false;
        fh->err = false;
        // if Append — immediately to the end
        if (has(mode, OpenMode::Append)) {
            LARGE_INTEGER li{ 0 };
            if (!::SetFilePointerEx(fh->h, li, nullptr, FILE_END)) fh->err=true;
        }
        out = fh;
        return true;
    }
    inline void close_file(file_handle* h) noexcept {
        if (!h) return;
        if (h->h != INVALID_HANDLE_VALUE) {
            ::CloseHandle(h->h);
            h->h = INVALID_HANDLE_VALUE;
        }
        heap_delete(h);
    }
    IO_NODISCARD inline usize read_file(file_handle* h, void* dst, usize bytes) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE || !dst || bytes == 0) return 0;
        h->eof = false;
        DWORD got = 0;
        BOOL ok = ::ReadFile(h->h, dst, (DWORD)bytes, &got, nullptr);
        if (!ok) { h->err=true; return 0; }
        if (got == 0) { h->eof = true; /* for the files it's typically EOF */ }
        return (usize)got;
    }
    IO_NODISCARD inline usize write_file(file_handle* h, const void* src, usize bytes) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE || !src || bytes == 0) return 0;
        h->eof = false;
        DWORD wrote = 0;
        BOOL ok = ::WriteFile(h->h, src, (DWORD)bytes, &wrote, nullptr);
        if (!ok) { h->err=true; return 0; }
        return (usize)wrote;
    }
    IO_NODISCARD inline bool flush_file(file_handle* h) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE) return false;
        if (!::FlushFileBuffers(h->h)) { h->err=true; return false; }
        return true;
    }
    IO_NODISCARD inline bool seek_file(file_handle* h, i64 offset, SeekWhence whence) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE) return false;
        DWORD move = FILE_BEGIN;
        if (whence == SeekWhence::Current) move = FILE_CURRENT;
        else if (whence == SeekWhence::End) move = FILE_END;

        LARGE_INTEGER li{};
        li.QuadPart = offset;
        BOOL ok = ::SetFilePointerEx(h->h, li, nullptr, move);
        if (!ok) { h->err=true; return false; }
        h->eof = false;
        return true;
    }
    IO_NODISCARD inline u64 tell_file(file_handle* h) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE) return 0;
        LARGE_INTEGER zero{ 0 };
        LARGE_INTEGER cur;
        BOOL ok = ::SetFilePointerEx(h->h, zero, &cur, FILE_CURRENT);
        if (!ok) { h->err=true; return 0; }
        return (u64)cur.QuadPart;
    }
    IO_NODISCARD inline u64 size_file(file_handle* h) noexcept {
        if (!h || h->h == INVALID_HANDLE_VALUE) return 0;
        LARGE_INTEGER sz;
        if (!::GetFileSizeEx(h->h, &sz)) { h->err=true; return 0; }
        return (u64)sz.QuadPart;
    }
    IO_NODISCARD inline bool is_eof(file_handle* h) noexcept { return h ? h->eof : true; }
    IO_NODISCARD inline bool has_error(file_handle* h) noexcept { return h ? h->err : true; }
    inline void clear_error(file_handle* h) noexcept { if (h) { h->err = false; h->eof = false; } }
#elif defined(__linux__)
struct file_handle {
        int  fd{ -1 };
        bool eof{ false };
        bool err{ false };
        OpenMode mode{ OpenMode::None };
    };

    static inline int flags_from_mode(OpenMode m) noexcept {
        const bool rd = has(m, OpenMode::Read);
        const bool wr = has(m, OpenMode::Write) || has(m, OpenMode::Append);

        int fl = 0;
        if (rd && wr) fl |= k_o_rdwr;
        else if (wr) fl |= k_o_wronly;
        else fl |= k_o_rdonly;

        if (has(m, OpenMode::Create))   fl |= k_o_creat;
        if (has(m, OpenMode::Truncate)) fl |= k_o_trunc;
        if (has(m, OpenMode::Append))   fl |= k_o_append;

        fl |= k_o_cloexec;
        return fl;
    }

    IO_NODISCARD inline bool open_file(file_handle*& out, char_view utf8_path, OpenMode mode) noexcept {
        out = nullptr;
        if (!utf8_path || utf8_path.empty()) return false;

        file_handle* fh = heap_new<file_handle>();
        if (!fh) return false;

        // NOTE: requires NUL-terminated path -> create temporary string
        io::string p;
        if (!p.append(utf8_path)) { heap_delete(fh); return false; }

        const int flags = flags_from_mode(mode);
        const int perm  = 0644;

        long r;
        do {
            r = sys4(k_sys_openat, (long)k_at_fdcwd, (long)(uintptr_t)p.c_str(), (long)flags, (long)perm);
        } while (is_err(r) && err_no(r) == k_eintr);

        if (is_err(r)) { heap_delete(fh); return false; }

        fh->fd = (int)r;
        fh->mode = mode;
        fh->eof = false;
        fh->err = false;
        out = fh;
        return true;
    }

    inline void close_file(file_handle* h) noexcept {
        if (!h) return;
        if (h->fd >= 0) {
            (void)sys1(k_sys_close, (long)h->fd);
            h->fd = -1;
        }
        heap_delete(h);
    }

    IO_NODISCARD inline usize read_file(file_handle* h, void* dst, usize bytes) noexcept {
        if (!h || h->fd < 0 || !dst || bytes == 0) return 0;
        h->eof = false;
        for (;;) {
            long r = sys3(k_sys_read, (long)h->fd, (long)(uintptr_t)dst, (long)bytes);
            if (r > 0) return (usize)r;
            if (r == 0) { h->eof = true; return 0; }
            const long e = err_no(r);
            if (e == k_eintr) continue;
            h->err = true;
            return 0;
        }
    }

    IO_NODISCARD inline usize write_file(file_handle* h, const void* src, usize bytes) noexcept {
        if (!h || h->fd < 0 || !src || bytes == 0) return 0;
        h->eof = false;

        usize pos = 0;
        while (pos < bytes) {
            long r = sys3(k_sys_write, (long)h->fd, (long)(uintptr_t)((const char*)src + pos), (long)(bytes - pos));
            if (r > 0) { pos += (usize)r; continue; }
            if (r == 0) break;
            const long e = err_no(r);
            if (e == k_eintr) continue;
            h->err = true;
            break;
        }
        return pos;
    }

    IO_NODISCARD inline bool flush_file(file_handle* h) noexcept {
        if (!h || h->fd < 0) return false;
        for (;;) {
            long r = sys1(k_sys_fsync, (long)h->fd);
            if (!is_err(r)) return true;
            const long e = err_no(r);
            if (e == k_eintr) continue;
            h->err = true;
            return false;
        }
    }

    IO_NODISCARD inline bool seek_file(file_handle* h, i64 offset, SeekWhence whence) noexcept {
        if (!h || h->fd < 0) return false;
        int w = k_seek_set;
        if (whence == SeekWhence::Current) w = k_seek_cur;
        else if (whence == SeekWhence::End) w = k_seek_end;

        long r;
        do {
            r = sys3(k_sys_lseek, (long)h->fd, (long)offset, (long)w);
        } while (is_err(r) && err_no(r) == k_eintr);

        if (is_err(r)) { h->err = true; return false; }
        h->eof = false;
        return true;
    }

    IO_NODISCARD inline u64 tell_file(file_handle* h) noexcept {
        if (!h || h->fd < 0) return 0;
        long r;
        do {
            r = sys3(k_sys_lseek, (long)h->fd, 0, (long)k_seek_cur);
        } while (is_err(r) && err_no(r) == k_eintr);

        if (is_err(r)) { h->err = true; return 0; }
        return (u64)r;
    }

    // ---- minimal kernel stat layout for size/type via newfstatat ----
    // We only need st_mode + st_size. Layout differs per arch; use statx for perfection,
    // but this minimal works for common kernels/ABIs. If you hit issues, we’ll switch to statx.
#if defined(IO_ARCH_X86_64)
    struct kstat {
        u64 st_dev; u64 st_ino; u64 st_nlink;
        u32 st_mode; u32 st_uid; u32 st_gid; u32 __pad0;
        u64 st_rdev;
        i64 st_size;
        i64 st_blksize;
        i64 st_blocks;
        u64 st_atime; u64 st_atime_nsec;
        u64 st_mtime; u64 st_mtime_nsec;
        u64 st_ctime; u64 st_ctime_nsec;
        i64 __unused[3];
    };
#elif defined(IO_ARCH_X86_32)
    // i386 is trickier; simplest: don’t support size_file/tell via stat here
    struct kstat { u32 _dummy; };
#endif

    static inline bool fstatat_basic(int dirfd, const char* cpath, int flags, u32& out_mode, u64& out_size) noexcept {
#if defined(IO_ARCH_X86_32)
#error "fstatat_basic(i386): Not implemented"
        (void)dirfd; (void)cpath; (void)flags; (void)out_mode; (void)out_size;
        return false;
#else
        kstat st{};
        long r;
        do {
            r = sys4(k_sys_newfstatat, (long)dirfd, (long)(uintptr_t)cpath, (long)(uintptr_t)&st, (long)flags);
        } while (is_err(r) && err_no(r) == k_eintr);
        if (is_err(r)) return false;
        out_mode = st.st_mode;
        out_size = (u64)st.st_size;
        return true;
#endif
    }

    IO_NODISCARD inline u64 size_file(file_handle* h) noexcept {
        if (!h || h->fd < 0) return 0;
#if defined(IO_ARCH_X86_32)
#error "size_file(i386): Not implemented"
        h->err = true;
        return 0;
#else
        u32 mode = 0; u64 sz = 0;
        // fstat(fd) can be done as fstatat(fd,"",AT_EMPTY_PATH) but we avoid extra flags now:
        // use /proc/self/fd? not freestanding. easiest: lseek end+restore.
        const u64 cur = tell_file(h);
        if (!seek_file(h, 0, SeekWhence::End)) return 0;
        const u64 end = tell_file(h);
        (void)seek_file(h, (i64)cur, SeekWhence::Begin);
        (void)mode;
        (void)sz;
        return end;
#endif
    }

    IO_NODISCARD inline bool is_eof(file_handle* h) noexcept { return h ? h->eof : true; }
    IO_NODISCARD inline bool has_error(file_handle* h) noexcept { return h ? h->err : true; }
    inline void clear_error(file_handle* h) noexcept { if (h) { h->err = false; h->eof = false; } }
#else
#  error "Not implemented"
#endif // _WIN32
} // namespace native
#endif // IO_IMPLEMENTATION


    // ========================================================================
    //                   native   F I L E S Y S T E M
    // ========================================================================

namespace native {
    IO_CONSTEXPR_VAR usize MAX_PATH_LENGTH = 260;
    IO_CONSTEXPR_VAR usize MAX_NAME_LENGTH = 256;

    enum class file_type : unsigned char {
        none,
        not_found,
        regular,
        directory,
        symlink,
        other,
        unknown
    };

    struct dir_handle; // opaque

    // Get type and size of a file/directory.
    IO_NODISCARD inline bool stat(char_view utf8_path, file_type& outType, u64* outSize) noexcept;
    IO_NODISCARD inline bool create_directory(char_view utf8_path) noexcept;

    // Delete EMPTY directory or a file
    IO_NODISCARD inline bool remove(char_view utf8_path) noexcept;
    IO_NODISCARD inline bool rename(char_view utf8_from, char_view utf8_to) noexcept;
    IO_NODISCARD inline bool current_directory(string& out_utf8) noexcept;

    // ----- Directory job -----

    // Returns nullptr on error.
    IO_NODISCARD inline dir_handle* open_dir(char_view utf8_path) noexcept;

    // Read next element of a dir.
    // Returns false, when no elements left or error occurred.
    IO_NODISCARD inline bool read_dir(dir_handle* h, string& outName, file_type& outType, u64& outSize) noexcept;
    inline void close_dir(dir_handle* h) noexcept;
} // namespace native

#if defined(IO_IMPLEMENTATION)
namespace native {

#if defined(_WIN32)
    inline file_type from_attrs(DWORD attrs) noexcept {
        if (attrs == INVALID_FILE_ATTRIBUTES) return file_type::not_found;
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) return file_type::directory;
        return file_type::regular;
    }
    // -------- dir_handle (no raw wchar_t*) --------
    struct dir_handle {
        HANDLE find{ INVALID_HANDLE_VALUE };
        WIN32_FIND_DATAW data{};
        bool first_pending{ false };

        wstring dir_prefix{};
        wstring search_pattern{};
    };

    static bool make_search_pattern(const wstring& dir, wstring& out_pat) noexcept {
        out_pat.clear();
        if (!out_pat.reserve(dir.size() + 3)) return false;
        if (!out_pat.append(dir.c_str())) return false;

        if (!dir.empty()) {
            wchar_t last = dir[dir.size() - 1];
            if (last != L'\\' && last != L'/') {
                if (!out_pat.push_back(L'\\')) return false;
            }
        }
        if (!out_pat.push_back(L'*')) return false;
        return true;
    }

    // ---------------- API implementation ----------------

    inline bool stat(char_view utf8_path, file_type& out_type, u64* out_size) noexcept {
        if (!utf8_path) {
            out_type = file_type::unknown;
            if (out_size) *out_size = 0;
            return false;
        }
        wstring w;

        if (!native::utf8_to_wide(utf8_path, w)) {
            out_type = file_type::unknown;
            if (out_size) *out_size = 0;
            return false;
        }
        WIN32_FILE_ATTRIBUTE_DATA fad{};
        if (!::GetFileAttributesExW(w.c_str(), GetFileExInfoStandard, &fad)) {
            out_type = file_type::not_found;
            if (out_size) *out_size = 0;
            return true;
        }
        out_type = from_attrs(fad.dwFileAttributes);

        if (out_size) {
            ULARGE_INTEGER li{};
            li.LowPart = fad.nFileSizeLow;
            li.HighPart = fad.nFileSizeHigh;
            *out_size = li.QuadPart;
        }
        return true;
    }

    inline bool create_directory(char_view utf8_path) noexcept {
        if (!utf8_path) return false;
        wstring w;
        if (!native::utf8_to_wide(utf8_path, w)) return false;
        return ::CreateDirectoryW(w.c_str(), nullptr) != 0;
    }

    inline bool remove(char_view utf8_path) noexcept {
        if (!utf8_path) return false;

        file_type ft{};
        u64 sz{};
        if (!stat(utf8_path, ft, &sz)) return false;

        wstring w;
        if (!native::utf8_to_wide(utf8_path, w)) return false;
        if (ft == file_type::directory)
            return ::RemoveDirectoryW(w.c_str()) != 0;
        return ::DeleteFileW(w.c_str()) != 0;
    }

    inline bool rename(char_view utf8_from, char_view utf8_to) noexcept {
        if (!utf8_from || !utf8_to) return false;

        wstring wfrom, wto;
        if (!native::utf8_to_wide(utf8_from, wfrom)) return false;
        if (!native::utf8_to_wide(utf8_to, wto)) return false;

        return ::MoveFileW(wfrom.c_str(), wto.c_str()) != 0;
    }

    inline bool current_directory(string& out_utf8) noexcept {
        out_utf8.clear();

        // 1) query required length (includes '\0')
        DWORD need = ::GetCurrentDirectoryW(0, nullptr);
        if (need == 0) return false;

        // 2) allocate wide buffer (size == need-1 characters, but storage includes '\0')
        wstring wdir;
        if (!wdir.resize(static_cast<usize>(need - 1))) return false;

        // 3) fill
        DWORD got = ::GetCurrentDirectoryW(need, wdir.data());
        if (got == 0) return false;
        // If path changed between calls, got may be >= need; handle by retry
        if (got >= need) {
            // got is required length excluding '\0' in some cases; safest: retry with got+1
            if (!wdir.resize(static_cast<usize>(got))) return false;
            DWORD got2 = ::GetCurrentDirectoryW(got + 1, wdir.data());
            if (got2 == 0 || got2 > got) return false;
        }

        // 4) convert to UTF-8
        return native::wide_to_utf8(wdir.c_str(), out_utf8);
    }

    inline dir_handle* open_dir(char_view utf8_path) noexcept {
        if (!utf8_path) return nullptr;

        dir_handle* h = heap_new<dir_handle>();
        if (!h) return nullptr;
        if (!native::utf8_to_wide(utf8_path, h->dir_prefix)) {
            heap_delete(h);
            return nullptr;
        }
        if (!make_search_pattern(h->dir_prefix, h->search_pattern)) {
            heap_delete(h);
            return nullptr;
        }
        h->find = ::FindFirstFileW(h->search_pattern.c_str(), &h->data);
        if (h->find == INVALID_HANDLE_VALUE) {
            heap_delete(h);
            return nullptr;
        }
        h->first_pending = true;
        return h;
    }

    inline bool read_dir(dir_handle* h,
                         string& out_name,
                         file_type& out_type,
                         u64& out_size) noexcept {
        if (!h) return false;

        WIN32_FIND_DATAW* data = &h->data;

        if (!h->first_pending) {
            if (!::FindNextFileW(h->find, data))
                return false;
        }
        else {
            h->first_pending = false;
        }
        // Convert filename to UTF-8 into string
        if (!native::wide_to_utf8(data->cFileName, out_name))
            out_name.clear(); // fallback empty
        out_type = from_attrs(data->dwFileAttributes);

        ULARGE_INTEGER li{};
        li.LowPart = data->nFileSizeLow;
        li.HighPart = data->nFileSizeHigh;
        out_size = li.QuadPart;

        return true;
    }


    inline void close_dir(dir_handle* h) noexcept {
        if (!h) return;
        if (h->find != INVALID_HANDLE_VALUE) ::FindClose(h->find);
        heap_delete(h);
    }

#elif defined(__linux__)
    // ---- mode bits ----
    IO_CONSTEXPR_VAR u32 k_s_ifmt  = 0170000;
    IO_CONSTEXPR_VAR u32 k_s_ifreg = 0100000;
    IO_CONSTEXPR_VAR u32 k_s_ifdir = 0040000;
    IO_CONSTEXPR_VAR u32 k_s_iflnk = 0120000;

    static inline file_type type_from_mode(u32 m) noexcept {
        const u32 t = (m & k_s_ifmt);
        if (t == k_s_ifreg) return file_type::regular;
        if (t == k_s_ifdir) return file_type::directory;
        if (t == k_s_iflnk) return file_type::symlink;
        return file_type::other;
    }

    // linux_dirent64 from kernel ABI
    struct linux_dirent64 {
        u64 d_ino;
        i64 d_off;
        u16 d_reclen;
        u8  d_type;
        char d_name[1];
    };

    IO_CONSTEXPR_VAR u8 k_dt_unknown = 0;
    IO_CONSTEXPR_VAR u8 k_dt_reg     = 8;
    IO_CONSTEXPR_VAR u8 k_dt_dir     = 4;
    IO_CONSTEXPR_VAR u8 k_dt_lnk     = 10;

    struct dir_handle {
        int fd{ -1 };
        u8  buf[4096];
        usize pos{ 0 };
        usize nread{ 0 };
    };

    inline bool stat(char_view utf8_path, file_type& out_type, u64* out_size) noexcept {
        out_type = file_type::unknown;
        if (out_size) *out_size = 0;
        if (!utf8_path) return false;
        io::string p{ utf8_path };
        if (p.empty()) return false;

        u32 mode = 0; u64 sz = 0;
#if defined(IO_ARCH_X86_32)
#error "stat(i386): Not implemented"
        out_type = file_type::unknown;
        return false;
#else
        if (!fstatat_basic(k_at_fdcwd, p.c_str(), k_at_symlink_nofollow, mode, sz)) {
            // could be not found
            out_type = file_type::not_found;
            return true;
        }
        out_type = type_from_mode(mode);
        if (out_size) *out_size = sz;
        return true;
#endif
    }

    inline bool create_directory(char_view utf8_path) noexcept {
        if (!utf8_path) return false;
        io::string p{ utf8_path };
        if (p.empty()) return false;
        const int perm = 0755;
        long r;
        do {
            r = sys3(k_sys_mkdirat, (long)k_at_fdcwd, (long)(uintptr_t)p.c_str(), (long)perm);
        } while (is_err(r) && err_no(r) == k_eintr);

        return !is_err(r);
    }

    inline bool remove(char_view utf8_path) noexcept {
        if (!utf8_path) return false;
        file_type ft{};
        u64 sz{};
        if (!stat(utf8_path, ft, &sz)) return false;
        if (ft == file_type::not_found) return true;

        io::string p{ utf8_path };
        if (p.empty()) return false;
        int flags = 0;
        if (ft == file_type::directory) flags |= k_at_removedir;

        long r;
        do {
            r = sys3(k_sys_unlinkat, (long)k_at_fdcwd, (long)(uintptr_t)p.c_str(), (long)flags);
        } while (is_err(r) && err_no(r) == k_eintr);

        return !is_err(r);
    }

    inline bool rename(char_view utf8_from, char_view utf8_to) noexcept {
        if (!utf8_from || !utf8_to) return false;

        io::string a{ utf8_from }, b{ utf8_to };
        if (a.empty() || b.empty()) return false;

        long r;
        do {
            // renameat(k_at_fdcwd, from, k_at_fdcwd, to)
            r = sys4(k_sys_renameat, (long)k_at_fdcwd, (long)(uintptr_t)a.c_str(), (long)k_at_fdcwd, (long)(uintptr_t)b.c_str());
        } while (is_err(r) && err_no(r) == k_eintr);

        return !is_err(r);
    }

    inline bool current_directory(string& out_utf8) noexcept {
        out_utf8.clear();

        // grow buffer until getcwd succeeds
        usize cap = 256;
        for (int it = 0; it < 8; ++it) {
            if (!out_utf8.resize(cap)) return false;
            long r = sys2(k_sys_getcwd, (long)(uintptr_t)out_utf8.data(), (long)cap);
            if (!is_err(r)) {
                // r = length including '\0' on success
                const usize n = (usize)r;
                if (n == 0) { out_utf8.clear(); return false; }
                if (n > 0) {
                    // remove '\0'
                    if (!out_utf8.resize(n-1)) return false;
                }
                return true;
            }

            const long e = err_no(r);
            if (e == k_erange) { cap *= 2; continue; }
            return false;
        }
        return false;
    }

    inline dir_handle* open_dir(char_view utf8_path) noexcept {
        if (!utf8_path ) return nullptr;
        io::string p{ utf8_path };
        if (p.empty()) return nullptr;

        const int flags = k_o_rdonly | k_o_directory | k_o_cloexec;
        long r;
        do {
            r = sys4(k_sys_openat, (long)k_at_fdcwd, (long)(uintptr_t)p.c_str(), (long)flags, (long)0);
        } while (is_err(r) && err_no(r) == k_eintr);
        if (is_err(r)) return nullptr;

        dir_handle* h = heap_new<dir_handle>();
        if (!h) { (void)sys1(k_sys_close, r); return nullptr; }

        h->fd = (int)r;
        h->pos = 0;
        h->nread = 0;
        return h;
    }

    static inline bool fstatat_name(dir_handle* h, const char* name_cstr, file_type& out_type, u64& out_size) noexcept {
        out_type = file_type::unknown;
        out_size = 0;

#if defined(IO_ARCH_X86_32)
#error "fstatat_name(i386): Not implemented"
        (void)h; (void)name_cstr;
        return false;
#else
        u32 mode = 0; u64 sz = 0;
        if (!fstatat_basic(h->fd, name_cstr, k_at_symlink_nofollow, mode, sz)) {
            out_type = file_type::unknown;
            out_size = 0;
            return true; // still "ok", just unknown
        }
        out_type = type_from_mode(mode);
        out_size = sz;
        return true;
#endif
    }

    inline bool read_dir(dir_handle* h, string& out_name, file_type& out_type, u64& out_size) noexcept {
        if (!h || h->fd < 0) return false;

        for (;;) {
            if (h->pos >= h->nread) {
                // refill
                long r;
                do {
                    r = sys3(k_sys_getdents64, (long)h->fd, (long)(uintptr_t)h->buf, (long)sizeof(h->buf));
                } while (is_err(r) && err_no(r) == k_eintr);
                if (is_err(r) || r == 0) return false;
                h->nread = (usize)r;
                h->pos = 0;
            }
            auto* d = (linux_dirent64*)(h->buf + h->pos);
            h->pos += (usize)d->d_reclen;

            const char* nm = d->d_name;
            if (!nm || nm[0] == 0) continue;
            if (nm[0] == '.' && (nm[1] == 0 || (nm[1] == '.' && nm[2] == 0))) continue;
            // copy name
            out_name.clear();
            // d_reclen includes full record, name is NUL-terminated
            if (!out_name.append(char_view{ nm, ::io::len(nm) })) return false;
            
            // type + size (prefer stat for correctness)
            (void)fstatat_name(h, nm, out_type, out_size);
            if (out_type == file_type::unknown) {
                // fallback from d_type if stat not available
                if      (d->d_type == k_dt_dir) out_type = file_type::directory;
                else if (d->d_type == k_dt_reg) out_type = file_type::regular;
                else if (d->d_type == k_dt_lnk) out_type = file_type::symlink;
                else                          out_type = file_type::unknown;
            }

            return true;
        }
    }

    inline void close_dir(dir_handle* h) noexcept {
        if (!h) return;
        if (h->fd >= 0) (void)sys1(k_sys_close, (long)h->fd);
        heap_delete(h);
    }

#endif // _WIN32

} // namespace native

#endif // IO_IMPLEMENTATION
} // namespace io

namespace fs {
#ifdef _WIN32
    IO_CONSTEXPR_VAR char sep = '\\';
#else
    IO_CONSTEXPR_VAR char sep = '/';
#endif

    template<io::usize N>
    static void path_join(io::char_view base, io::char_view leaf, io::StackOut<N>& out) noexcept {
        out.write(base.data(), base.size());

        const char last = out.storage.view()[out.storage.view().size() - 1];
        if (last != '/' && last != '\\') {
            out.write(&fs::sep, 1);
        }
        return out.write(leaf.data(), leaf.size());
    }
    static bool path_join(io::char_view base, io::char_view leaf, io::string& out) noexcept {
       out.clear();
       if (!out.append(base)) return false;
        
       if (!out.empty()) {
           const char last = out[out.size() - 1];
           if (last != '/' && last != '\\') {
               if (!out.push_back(fs::sep)) return false;
           }
       }
       return out.append(leaf);
    }
    static bool path_join(const io::string& base, io::char_view leaf, io::string& out) noexcept { return path_join(base.as_view(), leaf, out); }
    static bool path_join(io::char_view base, const io::string& leaf, io::string& out) noexcept { return path_join(base, leaf.as_view(), out); }
    static bool path_join(const io::string& base, const io::string& leaf, io::string& out) noexcept { return path_join(base.as_view(), leaf.as_view(), out); }
    // ------------------------------------------------------------
    //                      io::File — RAII
    // ------------------------------------------------------------
struct File {
    File() noexcept = default;
    inline explicit File(io::char_view path_utf8, io::OpenMode mode = io::OpenMode::Read) noexcept : _mode{ mode } { (void)io::native::open_file(_h, path_utf8, _mode); }
    inline explicit File(const io::string& path_utf8, io::OpenMode mode = io::OpenMode::Read) noexcept : File(path_utf8.as_view(), mode) {}
    inline ~File() noexcept { close(); }
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    File(File&& o) noexcept { *this = static_cast<File&&>(o); }
    File& operator=(File&& o) noexcept {
        if (this == &o) return *this;
        close();
        _h = o._h;        o._h = nullptr;
        _mode = o._mode;  o._mode = io::OpenMode::None;
        return *this;
    }
    // ---- lifetime ----
    IO_NODISCARD bool open(io::char_view path_utf8, io::OpenMode mode) noexcept {
        close();
        _mode = mode;
        return io::native::open_file(_h, path_utf8, mode);
    }
    IO_NODISCARD inline bool open(const io::string& path_utf8, io::OpenMode mode) noexcept { return open(path_utf8.as_view(), mode); }
    void close() noexcept {
        if (_h) {
            io::native::close_file(_h);
            _h = nullptr;
        }
        _mode = io::OpenMode::None;
    }
    IO_NODISCARD bool is_open() const noexcept { return _h != nullptr; }
    // ---- status like fstream ----
    IO_NODISCARD bool good() const noexcept { return _h && !io::native::has_error(_h); }
    IO_NODISCARD bool fail() const noexcept { return !_h || io::native::has_error(_h); }
    IO_NODISCARD bool eof() const noexcept { return _h ? io::native::is_eof(_h) : true; }
    void clear() noexcept { if (_h) io::native::clear_error(_h); }
    // ---- core i/o ----
    IO_NODISCARD io::usize read(io::view<char> dst) noexcept {
        if (!dst || !_h || !io::has(_mode, io::OpenMode::Read)) return 0;
        return io::native::read_file(_h, dst.data(), dst.size());
    }
    IO_NODISCARD io::usize write(io::char_view src) noexcept {
        if (!src || !_h) return 0;
        // allow Write or Append
        if (!io::has(_mode, io::OpenMode::Write) &&
            !io::has(_mode, io::OpenMode::Append))
            return 0;
        return io::native::write_file(_h, src.data(), src.size());
    }
    IO_NODISCARD bool flush() noexcept { return _h ? io::native::flush_file(_h) : false; }
    IO_NODISCARD bool seek(io::i64 offset, io::SeekWhence whence) noexcept { return _h ? io::native::seek_file(_h, offset, whence) : false; }
    IO_NODISCARD io::u64 tell() const noexcept { return _h ? io::native::tell_file(_h) : 0; }
    IO_NODISCARD io::u64 size() const noexcept { return _h ? io::native::size_file(_h) : 0; }
    // ---- convenience helpers ----
    IO_NODISCARD bool write_str(io::char_view v) noexcept { return write(v) == v.size(); }
    IO_NODISCARD bool write_line(io::char_view line) noexcept {
        if (write(line) != line.size()) return false;
        return write({ "\n", 1 }) == 1;
    }
    // reads line till '\n'; return `false` if empty, or EOF
    IO_NODISCARD bool read_line(io::string& out) noexcept {
        out.clear();
        if (!_h) return false;
        char c = 0;
        io::view<char> one{ &c, 1 };
        bool got_any = false;

        for (;;) {
            if (read(one) == 0) break; // eof or error
            got_any = true;
            if (c == '\n') break;
            if (c == '\r') {
                // peek 1 byte, undo if not '\n'
                io::u64 pos_after_cr = tell(); // Windows CRLF: if next is '\n' - read
                if (read(one) == 1) {
                    if (c != '\n') {
                        if (!seek(static_cast<io::i64>(pos_after_cr), io::SeekWhence::Begin)) return false;
                    }
                }
                break;
            }
            if (!out.push_back(c)) return false;
        }
        if (!got_any && eof()) return false;
        return got_any || !out.empty();
    }

    // read whole file into string
    IO_NODISCARD bool read_all(io::string& out) noexcept {
        out.clear();
        if (!_h) return false;
        io::u64 sz64 = size();
        // 1) known size: 1 alloc, 1 loop
        if (sz64 != 0) {
            if (sz64 > (io::u64)static_cast<io::usize>(-1)) return false;
            const io::usize sz = static_cast<io::usize>(sz64);
            if (!out.resize(sz)) return false;
            io::usize got = 0;
            while (got < sz) {
                io::char_view_mut chunk{ out.data()+got, sz-got };
                io::usize r = read(chunk);
                if (r == 0) break;
                got += r;
            }
            if (got < sz)
                if (!out.resize(got)) return false;
            return good() || eof();
        }
        // 2) unknown size: chunked
        io::string buf{};
        if (!buf.reserve(4096)) return false;
        for (;;) {
            if (read(buf.as_mut_view()) == 0) break;
            if (!out.append(buf.as_view())) return false;
        }
        return good() || eof();
    } // read_all


    IO_NODISCARD bool read_exact(void* dst, io::usize bytes) noexcept {
        if (!dst || bytes == 0) return false;
        auto* p = (char*)dst;
        io::usize left = bytes;
        while (left) {
            io::usize n = read(io::char_view_mut{ p, left });
            if (n == 0) return false; // eof or error
            p += n;
            left -= n;
        }
        return true;
    }

private:
    io::native::file_handle* _h{ nullptr };
    io::OpenMode _mode{ io::OpenMode::None };
};


    // ========================================================================
    //                         F I L E S Y S T E M
    // ========================================================================
    using file_type = io::native::file_type;
    struct directory_entry {
        io::char_view path;
        io::u64       size;
        file_type type;
    };
    
    struct directory_iterator {
        directory_iterator() noexcept = default;
    
        explicit directory_iterator(io::char_view dir) noexcept {
            if (!dir) { _at_end = true; return; }
    
            (void)_dir_storage.append(dir);
    
            _prefix_storage.clear();
            (void)_prefix_storage.append(_dir_storage.as_view());
            if (!_prefix_storage.empty()) {
                char last = _prefix_storage[_prefix_storage.size() - 1];
                if (last != '\\' && last != '/') (void)_prefix_storage.push_back(sep);
            }
            else {
                (void)_prefix_storage.push_back(sep);
            }
    
            _handle = io::native::open_dir(_dir_storage.as_view());
            _at_end = (_handle == nullptr);
            if (!_at_end) ++(*this);
        }
    
        ~directory_iterator() noexcept {
            if (_handle) {
                io::native::close_dir(_handle);
                _handle = nullptr;
            }
        }
    
        directory_iterator(const directory_iterator&) = delete;
        directory_iterator& operator=(const directory_iterator&) = delete;
    
        directory_iterator(directory_iterator&& other) noexcept { *this = static_cast<directory_iterator&&>(other); }
    
        directory_iterator& operator=(directory_iterator&& o) noexcept {
            if (this == &o) return *this;
            if (_handle) io::native::close_dir(_handle);
    
            _handle = o._handle; o._handle = nullptr;
            _entry = o._entry;
            _at_end = o._at_end;
    
            _dir_storage = static_cast<io::string&&>(o._dir_storage);
            _prefix_storage = static_cast<io::string&&>(o._prefix_storage);
            _name_storage = static_cast<io::string&&>(o._name_storage);
    
            o._at_end = true;
            o._entry = {};
            o._dir_storage.clear();
            o._prefix_storage.clear();
            o._name_storage.clear();
            return *this;
        }
    
        directory_iterator& operator++() noexcept {
            if (!_handle || _at_end) { _at_end = true; return *this; }
    
            file_type type{};
            io::u64 size{};
            _leaf_storage.clear();
            for (;;) {
                if (!io::native::read_dir(_handle, _leaf_storage, type, size)) {
                    _at_end = true;
                    _entry = {};
                    break;
                }
    
                if (_leaf_storage == "." || _leaf_storage == "..") continue;
    
                _name_storage.clear();
                (void)_name_storage.append(_prefix_storage.as_view());
                (void)_name_storage.append(_leaf_storage.as_view());
    
                _entry.path = _name_storage.as_view();
                _entry.type = type;
                _entry.size = size;
                break;
            }
            return *this;
        }
    
        IO_NODISCARD const directory_entry& operator*() const noexcept { return _entry; }
        IO_NODISCARD const directory_entry* operator->() const noexcept { return &_entry; }
        IO_NODISCARD bool is_end() const noexcept { return _at_end || !_handle; }
    
    private:
        io::native::dir_handle* _handle{ nullptr };
        directory_entry _entry{};
        bool _at_end{ true };
    
        io::string _dir_storage{};
        io::string _prefix_storage{}; // "<dir>\"
        io::string _name_storage{};   // "<dir>\file"
        io::string _leaf_storage{};   // "file"
    }; // struct directory_iterator
    
    // ---- simple abstractions over native API ----
    IO_NODISCARD static inline bool exists(io::char_view p) noexcept {
        if (!p) return false;
        file_type t{};
        io::u64 size{};
        if (!io::native::stat(p, t, &size)) return false;
        return t != file_type::not_found;
    }
    IO_NODISCARD static inline bool exists(const io::string& r) noexcept { return exists(r.as_view()); }
    IO_NODISCARD static inline file_type status(io::char_view p, io::u64* out_size = nullptr) noexcept {
        if (!p) {
            if (out_size) *out_size = 0;
            return file_type::unknown;
        }
        file_type t{};
        io::u64 size{};
        if (!io::native::stat(p, t, &size)) return file_type::unknown;
        if (out_size) *out_size = size;
        return t;
    }
    IO_NODISCARD static inline file_type status(const io::string& r, io::u64* out_size = nullptr) noexcept { return status(r.as_view()); }
    IO_NODISCARD static inline bool is_directory(io::char_view p) noexcept { io::u64 size{}; return status(p, &size) == file_type::directory; }
    IO_NODISCARD static inline bool is_directory(const io::string& r) noexcept { return is_directory(r.as_view()); }
    IO_NODISCARD static inline bool is_regular_file(io::char_view p) noexcept { io::u64 size{}; return status(p, &size) == file_type::regular; }
    IO_NODISCARD static inline bool is_regular_file(const io::string& r) noexcept { return is_regular_file(r.as_view()); }
    IO_NODISCARD static inline io::u64 file_size(io::char_view p) noexcept { io::u64 size{}; if (status(p, &size) == file_type::regular) return size; return 0; }
    IO_NODISCARD static inline io::u64 file_size(const io::string& r) noexcept { return file_size(r.as_view()); }
    IO_NODISCARD static inline bool create_directory(io::char_view p) noexcept { return io::native::create_directory(p); }
    IO_NODISCARD static inline bool create_directory(const io::string& r) noexcept { return create_directory(r.as_view()); }
    IO_NODISCARD static inline bool remove(io::char_view p) noexcept { return io::native::remove(p); }
    IO_NODISCARD static inline bool remove(const io::string& r) noexcept { return remove(r.as_view()); }
    IO_NODISCARD static inline bool rename(io::char_view from, io::char_view to) noexcept { return io::native::rename(from, to); }
    IO_NODISCARD static inline bool rename(const io::string& from, const io::string& to) noexcept { return rename(from.as_view(), to.as_view()); }
    IO_NODISCARD static inline bool current_directory(io::string& out_utf8) noexcept { return io::native::current_directory(out_utf8); }
} // namespace fs

namespace io {
    // ------------------------------------------------------------------------
    //                         S Y S C A L L S
    // ------------------------------------------------------------------------
#ifdef IO_IMPLEMENTATION
    void* alloc(usize bytes) noexcept {
        if (bytes == 0) return nullptr;
#if defined(_WIN32)
        return ::VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#elif defined(__linux__)
        using namespace native;
        // Store size in front of user pointer so free() doesn't need size.
        const usize total = bytes + sizeof(usize);

    #if defined(IO_ARCH_X86_64)
        long r = sys6(k_sys_mmap,
            /*addr*/ 0,
            (long)total,
            /*prot*/ (long)(k_prot_read | k_prot_write),
            /*flags*/(long)(k_map_private | k_map_anonymous),
            /*fd*/   (long)-1,
            /*off*/  (long)0
        );
        if (is_err(r)) return nullptr;
        void* base = (void*)(uintptr_t)r;

    #elif defined(IO_ARCH_X86_32)
        // mmap2 expects offset in 4096-byte pages; offset=0 ok.
        long r = sys6(k_sys_mmap2,
            /*addr*/ 0,
            (long)total,
            /*prot*/ (long)(k_prot_read | k_prot_write),
            /*flags*/(long)(k_map_private | k_map_anonymous),
            /*fd*/   (long)-1,
            /*pgoff*/(long)0
        );
        if (is_err(r)) return nullptr;
        void* base = (void*)(uintptr_t)r;
    #endif

        *reinterpret_cast<usize*>(base) = bytes;
        return reinterpret_cast<char*>(base) + sizeof(usize);
#endif
    }
    void free(void* ptr) noexcept {
        if (!ptr) return;
#if defined(_WIN32)
        ::VirtualFree(ptr, 0, MEM_RELEASE);
#elif defined(__linux__)
        using namespace native;
        void* base = reinterpret_cast<char*>(ptr) - sizeof(usize);
        const usize bytes = *reinterpret_cast<usize*>(base);
        const usize total = bytes + sizeof(usize);

        long r;
        do {
        #if defined(IO_ARCH_X86_64)
            r = sys2(k_sys_munmap, (long)(uintptr_t)base, (long)total);
        #else
            r = sys2(k_sys_munmap, (long)(uintptr_t)base, (long)total);
        #endif
        } while (is_err(r) && err_no(r) == k_eintr);
        (void)r; // ignore errors in free()
#endif
    }

#if defined(__linux__)
    [[noreturn]]
#endif
    void exit_process(int error_code) noexcept {
#if defined(_WIN32)
        ::ExitProcess(static_cast<UINT>(error_code));
#elif defined(__linux__)
    using namespace native;
    #if defined(IO_ARCH_X86_64)
        (void)sys1(k_sys_exit, (long)error_code);
    #else
        (void)sys1(k_sys_exit, (long)error_code);
    #endif
        for (;;) { asm volatile("" ::: "memory"); }
#endif
    }

    void sleep_ms(unsigned ms) noexcept {
#if defined(_WIN32)
        ::Sleep(ms);
#elif defined(__linux__)
        using namespace native;
        // nanosleep({sec,nsec})
        timespec req{};
        req.tv_sec  = (long)(ms / 1000u);
        req.tv_nsec = (long)((ms % 1000u) * 1000000u);
        for (;;) {
            long r = sys2(k_sys_nanosleep, (long)(uintptr_t)&req, (long)0 /*rem*/);
            if (!is_err(r)) return;
            if (err_no(r) == k_eintr) continue;
            return;
        }
#endif
    }

    IO_NODISCARD inline u64 monotonic_ms() noexcept {
#if defined(_WIN32)
        static volatile u32 qpc_freq = 0;
        static volatile u32 ms_per_tick_q16 = 0; // (1000<<16)/freq
        u32 fr = qpc_freq;
        u32 k = ms_per_tick_q16;
        if (fr == 0) {
            LARGE_INTEGER li{};
            ::QueryPerformanceFrequency(&li);
            fr = (u32)li.QuadPart; // QPC freq practically fits in 32 bits
            if (fr == 0) return 0;

            // k = (1000<<16)/freq
            k = io::div_u64_u32(1000ull << 16, fr);

            qpc_freq = fr; // begin: same value
            ms_per_tick_q16 = k;
        }

        LARGE_INTEGER c{};
        ::QueryPerformanceCounter(&c);

        u64 ticks = (u64)c.QuadPart;

        // ms = (ticks * k_q16) >> 16
        const u32 lo = (u32)(ticks & 0xFFFFFFFFu);
        const u32 hi = (u32)(ticks >> 32);

        const u64 prod_lo = (u64)lo * (u64)k;        // 32x32->64
        const u64 prod_hi = (u64)hi * (u64)k;        // 32x32->64
        const u64 prod = prod_lo + (prod_hi << 32);  // (ticks*k)

        return prod >> 16;

#elif defined(__linux__)
        using namespace native;
        timespec ts{};
        long r;
        do {
            r = sys2(k_sys_clock_gettime, (long)1 /*CLOCK_MONOTONIC*/, (long)(uintptr_t)&ts);
        } while (is_err(r) && err_no(r) == k_eintr);

        if (is_err(r)) return 0;
        return (u64)ts.tv_sec * 1000ull + (u64)ts.tv_nsec / 1000000ull;
#endif
    }
    IO_NODISCARD inline u64 monotonic_us() noexcept {
#if defined(_WIN32)
        static volatile u32 qpc_freq = 0;
        static volatile u32 us_per_tick_q16 = 0; // (1000000<<16)/freq

        u32 fr = qpc_freq;
        u32 k = us_per_tick_q16;

        if (fr == 0) {
            LARGE_INTEGER li{};
            ::QueryPerformanceFrequency(&li);

            fr = (u32)li.QuadPart; // QPC frequency practically fits in 32 bits
            if (fr == 0) return 0;

            // k = (1000000 << 16) / freq
            k = io::div_u64_u32(1000000ull << 16, fr);

            qpc_freq = fr;
            us_per_tick_q16 = k;
        }

        LARGE_INTEGER c{};
        ::QueryPerformanceCounter(&c);

        const u64 ticks = (u64)c.QuadPart;

        // us = (ticks * k_q16) >> 16
        const u32 lo = (u32)(ticks & 0xFFFFFFFFu);
        const u32 hi = (u32)(ticks >> 32);

        const u64 prod_lo = (u64)lo * (u64)k;
        const u64 prod_hi = (u64)hi * (u64)k;
        const u64 prod = prod_lo + (prod_hi << 32);

        return prod >> 16;

#elif defined(__linux__)
        using namespace native;

        timespec ts{};
        long r;
        do {
            r = sys2(k_sys_clock_gettime, (long)1 /* CLOCK_MONOTONIC */, (long)(uintptr_t)&ts);
        } while (is_err(r) && err_no(r) == k_eintr);

        if (is_err(r)) return 0;

        return (u64)ts.tv_sec * 1000000ull + (u64)ts.tv_nsec / 1000ull;

#else
#   error "Not implemented"
#endif
    }
    // ------------------------------------------------------------------------
    // Crypto: secure memory wipe (must not be optimized away)
    // ------------------------------------------------------------------------
    void secure_zero(void* p, usize size) noexcept {
        if (!p || size == 0) return;
#if defined(_WIN32)
        ::SecureZeroMemory(p, (SIZE_T)size); // is guaranteed not to be optimized out.
#elif defined(__linux__)
        // Volatile byte store + compiler barrier.
        volatile unsigned char* v = (volatile unsigned char*)p;
        while (size--) *v++ = 0;
#   if defined(__GNUC__) || defined(__clang__)
        __asm__ __volatile__("" : : "r"(p) : "memory");
#   endif
#endif
    }
#endif // IO_IMPLEMENTATION

    // ------------------------------------------------------------------------
    // Crypto: OS entropy
    // ------------------------------------------------------------------------
    bool os_entropy(void* dst, usize size) noexcept {
        if (!dst || size == 0) return true;

#ifdef IO_IMPLEMENTATION
#if defined(_WIN32)
        // System-preferred CSPRNG (BCryptGenRandom).
        const NTSTATUS st = ::BCryptGenRandom(
            nullptr,
            (PUCHAR)dst,
            (ULONG)size,
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
        );
        return st >= 0;

#elif defined(__linux__)
        fs::File f{ char_view{"/dev/urandom"}, OpenMode::Read };
        return f.is_open() && f.read_exact(dst, size);
#else
#   error "Not implemented"
#endif
#endif // IO_IMPLEMENTATION
    }

} // namespace io


#ifdef IO_IMPLEMENTATION

#if defined(__linux__)
    [[noreturn]] inline void trap() noexcept {
        __builtin_trap();
        for (;;) {}
    }
    [[noreturn]] inline void assert_fail(const char* expr, const char* file, int line, const char* func) noexcept {
        (void)expr; (void)file; (void)line; (void)func;
#if defined(IO_TERMINAL)
        io::out << "ASSERT: " << expr << " @ " << file << ":" << (io::u32)line << " (" << func << ")\n";
#endif
        trap();
    }
#   ifndef IO_ASSERT
#      define IO_ASSERT(x) do { if (!(x)) assert_fail(#x, __FILE__, __LINE__, __func__); } while (0)
#   endif
#   ifndef assert
#      define assert(x) IO_ASSERT(x)
#   endif
#endif // __linux__

#if !defined(_DEBUG) || !defined(IO_HAS_STD)
#   ifdef IO_ASSERT
#      undef IO_ASSERT
#      define IO_ASSERT
#   endif
#   ifdef assert
#      undef assert
#      define assert
#   endif
#endif

#if defined(IO_HAS_STD)
#   include <new> // std::nothrow_t
#endif // IO_HAS_STD

#if !defined(IO_HAS_STD) && defined(__linux__)
#if !defined(HI_GUI_APP)
extern "C" {
    int main(); // Declare user's main (in same TU will exist)
    __attribute__((noreturn, used, visibility("default")))
    void _start() noexcept { // Entry point (ELF)
        // NOTE: without crt0 there're no argv/envp. If needed — parse stack manually.
        const int rc = main();
        io::exit_process(rc);
    }
}
#endif // !HI_GUI_APP
#endif // !IO_HAS_STD && __linux__

#if !defined(IO_HAS_STD)
    namespace io {
        struct nothrow_t { explicit constexpr nothrow_t(int) {} };
        constexpr nothrow_t nothrow{ 0 };
    } // namespace io
#endif

// ------------------------- new -------------------------
void* IO_CDECL operator new(io::usize bytes) {
    if (void* p = io::alloc_aligned(bytes ? bytes : 1, alignof(void*))) return p;
    // freestanding: no exceptions => terminate-style
#if defined(_MSC_VER)
    __debugbreak();
#else
    trap();
#endif
    return nullptr;
}
void* IO_CDECL operator new[](io::usize bytes) { return ::operator new(bytes); }

#if defined(IO_HAS_STD)
void* IO_CDECL operator new(io::usize bytes, const std::nothrow_t&) noexcept { return io::alloc_aligned(bytes ? bytes : 1, alignof(void*)); }
void* IO_CDECL operator new[](io::usize bytes, const std::nothrow_t&) noexcept { return io::alloc_aligned(bytes ? bytes : 1, alignof(void*)); }
#else
void* IO_CDECL operator new(io::usize bytes, const io::nothrow_t&) noexcept { return io::alloc_aligned(bytes ? bytes : 1, alignof(void*)); }
void* IO_CDECL operator new[](io::usize bytes, const io::nothrow_t&) noexcept { return io::alloc_aligned(bytes ? bytes : 1, alignof(void*)); }
#endif

// ------------------------- delete -------------------------
void IO_CDECL operator delete(void* p) noexcept { if (p) io::free_aligned(p); }
void IO_CDECL operator delete[](void* p) noexcept { if (p) io::free_aligned(p); }
#if defined(_MSC_VER) && defined(IO_ARCH_X86_32) // MSVC sized-delete
    void IO_CDECL operator delete(void* p, unsigned int) noexcept { if (p) io::free_aligned(p); }
    void IO_CDECL operator delete[](void* p, unsigned int) noexcept { if (p) io::free_aligned(p); }
#else
    void IO_CDECL operator delete(void* p, io::usize) noexcept { if (p) io::free_aligned(p); }
    void IO_CDECL operator delete[](void* p, io::usize) noexcept { if (p) io::free_aligned(p); }
#endif // x86
#endif // IO_IMPLEMENTATION