cmake_minimum_required(VERSION 3.20)
project(ZetaLatency LANGUAGES CXX)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ZL_BUILD_TESTS "Build unit tests (GoogleTest)" ON)
option(ZL_BUILD_BENCH "Build benchmark harness binary" ON)
option(ZL_USE_GOOGLE_BENCHMARK "Build Google Benchmark target (optional)" OFF)
option(ZL_ENABLE_LTO "Enable link-time optimization (Release only)" OFF)

include(CheckIPOSupported)
if(ZL_ENABLE_LTO)
  check_ipo_supported(RESULT lto_ok OUTPUT lto_err)
  if(lto_ok)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  else()
    message(WARNING "IPO/LTO not supported: ${lto_err}")
  endif()
endif()

# Git SHA for provenance (best-effort)
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  RESULT_VARIABLE _git_res
  OUTPUT_VARIABLE ZL_GIT_SHA
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(NOT _git_res EQUAL 0 OR "${ZL_GIT_SHA}" STREQUAL "")
  set(ZL_GIT_SHA "unknown")
endif()

add_library(zetalatency STATIC
  cpp/common/affinity.cpp
  cpp/common/sysinfo.cpp
  cpp/perf/perf_events.cpp
)
target_include_directories(zetalatency PUBLIC ${CMAKE_SOURCE_DIR}/cpp)
target_link_libraries(zetalatency PUBLIC Threads::Threads)
target_compile_definitions(zetalatency PUBLIC ZL_GIT_SHA="${ZL_GIT_SHA}")

if(UNIX AND NOT APPLE)
  target_compile_definitions(zetalatency PUBLIC ZL_LINUX=1)
endif()

# Warnings
if(MSVC)
  target_compile_options(zetalatency PUBLIC /W4)
else()
  target_compile_options(zetalatency PUBLIC -Wall -Wextra -Wpedantic)
endif()

if(ZL_BUILD_BENCH)
  add_executable(ZetaLatency_bench
    cpp/bench/main.cpp
  )
  target_link_libraries(ZetaLatency_bench PRIVATE zetalatency)
endif()

# GoogleTest (FetchContent)
if(ZL_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_executable(ZetaLatency_tests
    cpp/tests/test_ring_buffer.cpp
  )
  target_link_libraries(ZetaLatency_tests PRIVATE zetalatency GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(ZetaLatency_tests)
endif()

# Optional: Google Benchmark
if(ZL_USE_GOOGLE_BENCHMARK)
  include(FetchContent)
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.0
  )
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(benchmark)

  add_executable(ZetaLatency_gbench
    cpp/bench/google_benchmark.cpp
  )
  target_link_libraries(ZetaLatency_gbench PRIVATE zetalatency benchmark::benchmark)
endif()
