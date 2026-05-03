cmake_minimum_required(VERSION 3.20)

project(
    fpga_gated_execution_platform
    VERSION 0.1.0
    DESCRIPTION "C++20 ITCH/OUCH replay, order book, risk, and execution platform with FPGA-style guardrail gating"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Global C++ settings
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for editors, clangd, and tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Project options
# ---------------------------------------------------------------------------

option(FGEP_BUILD_TESTS "Build unit and integration tests" ON)
option(FGEP_BUILD_APPS "Build demo applications" ON)
option(FGEP_BUILD_BENCHMARKS "Build benchmark executables" OFF)
option(FGEP_ENABLE_WARNINGS "Enable strict compiler warnings" ON)

# ---------------------------------------------------------------------------
# Main library target
# ---------------------------------------------------------------------------
#
# This starts as an INTERFACE library so the repository can configure before
# implementation files exist. As the project grows, this can become a normal
# static library with real source files.

add_library(fgep INTERFACE)

target_include_directories(
    fgep
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(
    fgep
    INTERFACE
        cxx_std_20
)

if(FGEP_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(fgep INTERFACE /W4 /permissive-)
    else()
        target_compile_options(
            fgep
            INTERFACE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# Optional subdirectories
# ---------------------------------------------------------------------------
include(cmake/ProjectOptions.cmake)
include(cmake/CompilerWarnings.cmake)
include(cmake/Sanitizers.cmake)
include(CTest)

if(FGEP_BUILD_TESTS AND EXISTS "${PROJECT_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()

if(FGEP_BUILD_APPS AND EXISTS "${PROJECT_SOURCE_DIR}/apps/CMakeLists.txt")
    add_subdirectory(apps)
endif()

if(FGEP_BUILD_BENCHMARKS AND EXISTS "${PROJECT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(benchmarks)
endif()

# ---------------------------------------------------------------------------
# Status summary
# ---------------------------------------------------------------------------

message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build tests: ${FGEP_BUILD_TESTS}")
message(STATUS "Build apps: ${FGEP_BUILD_APPS}")
message(STATUS "Build benchmarks: ${FGEP_BUILD_BENCHMARKS}")

