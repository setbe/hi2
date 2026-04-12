cmake_minimum_required(VERSION 3.22)

project(abcd LANGUAGES CXX)

option(LEGACY_CPP14 "Use legacy C++14 compiler (default ON)" ON)
option(ABCD_BUILD_TESTS "Build tests for layer a" ON)

if(LEGACY_CPP14)
    set(CMAKE_CXX_STANDARD 14)
else()
    set(CMAKE_CXX_STANDARD 20)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(abcd_add_header_only target layer_dir)
    file(GLOB_RECURSE layer_headers CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/${layer_dir}/*.hpp"
    )

    add_library(${target} INTERFACE)
    target_include_directories(${target} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    )
    target_sources(${target} INTERFACE ${layer_headers})
endfunction()

# -----------------------------------------------------------------------------
# Header-only layer targets
# -----------------------------------------------------------------------------

abcd_add_header_only(a_layer a)
abcd_add_header_only(b_layer b)
abcd_add_header_only(c_layer c)
abcd_add_header_only(d_layer d)

target_link_libraries(b_layer INTERFACE a_layer)
target_link_libraries(c_layer INTERFACE a_layer b_layer)
target_link_libraries(d_layer INTERFACE a_layer b_layer c_layer)

# -----------------------------------------------------------------------------
# Tests (layer a)
# -----------------------------------------------------------------------------

if(ABCD_BUILD_TESTS)
    include(CTest)

    add_executable(test_a_compiletime
        examples/tests/a/test_a_compiletime.cpp
    )
    target_link_libraries(test_a_compiletime PRIVATE a_layer)

    add_executable(test_a_runtime
        examples/tests/a/test_a_runtime.cpp
    )
    target_link_libraries(test_a_runtime PRIVATE a_layer)
    target_include_directories(test_a_runtime PRIVATE
        ${CMAKE_SOURCE_DIR}/examples/tests
    )

    if(BUILD_TESTING)
        add_test(NAME a.compiletime COMMAND test_a_compiletime)
        add_test(NAME a.runtime COMMAND test_a_runtime)
    endif()
endif()

message(STATUS "Configured ${PROJECT_NAME} (C++${CMAKE_CXX_STANDARD})")
