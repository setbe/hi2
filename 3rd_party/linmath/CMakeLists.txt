cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------------------
# Custom configs for Visual Studio (BEFORE project())
# ---------------------------------------------------------------------------
set(LINMATH_CUSTOM_CONFIGS "ReleaseMini;ReleaseNoConsole;ReleaseMiniNoConsole")

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_CONFIGURATION_TYPES
        "Debug;Release;${LINMATH_CUSTOM_CONFIGS}"
        CACHE STRING "Available build configurations" FORCE
    )
endif()

project(freestanding_linmath LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Seed missing per-config variables (AFTER project())
# ---------------------------------------------------------------------------
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    foreach(cfg IN ITEMS ReleaseMini ReleaseNoConsole ReleaseMiniNoConsole)
        string(TOUPPER "${cfg}" CFGU)
        foreach(var IN ITEMS
            CMAKE_C_FLAGS
            CMAKE_CXX_FLAGS
            CMAKE_EXE_LINKER_FLAGS
        )
            if(NOT DEFINED ${var}_${CFGU})
                set(${var}_${CFGU} "${${var}_RELEASE}" CACHE STRING "" FORCE)
            endif()
        endforeach()
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(LEGACY_CPP14 "Use legacy C++14 compiler" ON)
option(BUILD_SHARED_LIBS "Build shared libs" OFF)
option(LINMATH_BENCH_NO_SIMD "Benchmarks without SIMD" ON)
option(LINMATH_BENCH_SIMD "Benchmarks with full SIMD" ON)

# ---------------------------------------------------------------------------
# Language standard
# ---------------------------------------------------------------------------
if(LEGACY_CPP14)
    set(CMAKE_CXX_STANDARD 14)
else()
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Freestanding configs
# ---------------------------------------------------------------------------
set(LINMATH_FREESTANDING_CONFIGS
    ReleaseMini
    ReleaseNoConsole
    ReleaseMiniNoConsole
)

set(_GE_FREESTANDING
    "$<OR:$<CONFIG:ReleaseMini>,$<CONFIG:ReleaseNoConsole>,$<CONFIG:ReleaseMiniNoConsole>>"
)

# ---------------------------------------------------------------------------
# Compiler flags (copied from stb fork)
# ---------------------------------------------------------------------------
set(msvc_common_compile
    "/permissive-"
    "/utf-8"
    "/diagnostics:column"
    "/nologo"
)

set(msvc_freestanding_base
    "/TP"
    "/GS-"
    "/GR-"
    "/Zl"
    "/Oi-"
    "/EHs-"
    "/EHc-"
    "/Gy"
    "/Zc:inline"
    "/Zc:wchar_t"
    "/sdl-"
    "/D_HAS_EXCEPTIONS=0"
    "/D;NDEBUG"
)

set(msvc_freestanding_full_opt "/O2" "/Ob2")
set(msvc_freestanding_mini_opt "/O1" "/Os")

set(msvc_debug_compile "/Zi" "/Od" "/GR")

set(unix_freestanding_base
    "-ffunction-sections"
    "-fdata-sections"
    "-fno-exceptions"
    "-fno-rtti"
    "-fomit-frame-pointer"
)

set(unix_debug_compile "-O0" "-g")

# ---------------------------------------------------------------------------
# Include root
# ---------------------------------------------------------------------------
set(PROJECT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# Helper functions (same philosophy as stb)
# ---------------------------------------------------------------------------

# common
function(lm_apply_common target)
    target_include_directories(${target} PRIVATE
        $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>
    )

    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:${msvc_common_compile}>
    )
endfunction()

# freestanding
function(lm_apply_freestanding target)
    target_compile_options(${target} PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,${_GE_FREESTANDING}>:${msvc_freestanding_base}>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:ReleaseNoConsole>>:${msvc_freestanding_full_opt}>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:ReleaseMini>>:${msvc_freestanding_mini_opt}>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:ReleaseMiniNoConsole>>:${msvc_freestanding_mini_opt}>
    )

    if(MSVC)
        set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY
            "$<IF:${_GE_FREESTANDING},MultiThreaded,$<IF:$<CONFIG:Debug>,MultiThreadedDebugDLL,MultiThreadedDLL>>"
        )
    endif()
endfunction()

# NO SIMD
function(lm_apply_no_simd target)
    target_compile_definitions(${target} PRIVATE LMATH_FORCE_NO_SIMD)

    if(MSVC)
        target_compile_options(${target} PRIVATE /arch:IA32)
    else()
        target_compile_options(${target} PRIVATE -mno-sse -mno-avx -mno-neon)
    endif()
endfunction()

# FULL SIMD
function(lm_apply_full_simd target)
    if(MSVC)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            target_compile_options(${target} PRIVATE /arch:AVX2)
        else()
            target_compile_options(${target} PRIVATE /arch:SSE2)
        endif()
    else()
        target_compile_options(${target} PRIVATE -march=native)
    endif()
endfunction()

# Debug
function(lm_apply_debug target)
    target_compile_options(${target} PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:${msvc_debug_compile}>
        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<CONFIG:Debug>>:${unix_debug_compile}>
    )
endfunction()

# No console
function(lm_no_console target)
    if(MSVC)
        target_link_options(${target} PRIVATE
            $<$<CONFIG:ReleaseNoConsole>:/SUBSYSTEM:WINDOWS>
            $<$<CONFIG:ReleaseMiniNoConsole>:/SUBSYSTEM:WINDOWS>
            $<$<OR:$<CONFIG:ReleaseNoConsole>,$<CONFIG:ReleaseMiniNoConsole>>:/ENTRY:WinMain>
        )
    endif()
endfunction()

# add header
function(lm_add_header_only name)
    add_library(${name} INTERFACE)
    target_include_directories(${name} INTERFACE
        $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>
    )
    target_sources(${name} INTERFACE ${ARGN})
endfunction()

# add exe
function(lm_add_test_exe name)
    set(oneValueArgs CPP)
    set(multiValueArgs HEADERS LIBS)
    cmake_parse_arguments(T "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${name} ${T_CPP} ${T_HEADERS})
    lm_no_console(${name})

    if(T_LIBS)
        target_link_libraries(${name} PRIVATE ${T_LIBS})
    endif()

    lm_apply_common(${name})
    lm_apply_freestanding(${name})
    lm_apply_debug(${name})
endfunction()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(LINMATH_HEADERS
    "linmath/detail/feature_detection.hpp"
    "linmath/detail/simd_integration.hpp"

    "linmath/libc_integration.hpp"
    "linmath/vec.hpp"
    "linmath/mat.hpp"
    "linmath/quat.hpp"
)

# ---------------------------------------------------------------------------
# Header-only target
# ---------------------------------------------------------------------------
lm_add_header_only(linmath ${LINMATH_HEADERS})

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

lm_add_test_exe(linmath_test_byte_diff
    CPP "test/test_byte_diff.cpp"
    HEADERS ${LINMATH_HEADERS}
            "3rd-party/linmath.h" # keep original copy for tests
            "test/compile_time.hpp"
    LIBS linmath
)

# NO SIMD
if(LINMATH_BENCH_NO_SIMD)
    lm_add_test_exe(linmath_bench_no_simd
        CPP "bench/bench_no_simd.cpp"
        HEADERS ${LINMATH_HEADERS}
                "3rd-party/linmath.h"
        LIBS linmath
    )

    lm_apply_no_simd(linmath_bench_no_simd)
endif()

# with SIMD
if(LINMATH_BENCH_SIMD)
    lm_add_test_exe(linmath_bench_simd
        CPP "bench/bench_simd.cpp"
        HEADERS ${LINMATH_HEADERS}
        LIBS linmath
    )

    lm_apply_full_simd(linmath_bench_simd)
endif()



# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
message(STATUS "Configured ${PROJECT_NAME}")
message(STATUS "Freestanding configs: ${LINMATH_FREESTANDING_CONFIGS}")
