# Optional third-party dependency discovery for FGEP.
#
# This file intentionally does not download dependencies.
# Dependencies should be installed by the developer, package manager, or CI image.

set(FGEP_GOOGLETEST_AVAILABLE OFF)
set(FGEP_GOOGLE_BENCHMARK_AVAILABLE OFF)

if(FGEP_ENABLE_GOOGLETEST)
    find_package(GTest CONFIG QUIET)

    if(NOT GTest_FOUND)
        find_package(GTest QUIET)
    endif()

    if(TARGET GTest::gtest_main)
        set(FGEP_GOOGLETEST_AVAILABLE ON)
        message(STATUS "GoogleTest: available")
    else()
        message(WARNING "FGEP_ENABLE_GOOGLETEST=ON, but GoogleTest was not found. GoogleTest targets will be skipped.")
        message(WARNING "Ubuntu example: sudo apt install libgtest-dev")
    endif()
else()
    message(STATUS "GoogleTest: disabled")
endif()

if(FGEP_ENABLE_GOOGLE_BENCHMARK)
    find_package(benchmark CONFIG QUIET)

    if(TARGET benchmark::benchmark_main)
        set(FGEP_GOOGLE_BENCHMARK_AVAILABLE ON)
        message(STATUS "Google Benchmark: available")
    else()
        message(WARNING "FGEP_ENABLE_GOOGLE_BENCHMARK=ON, but Google Benchmark was not found. Benchmark targets will be skipped.")
        message(WARNING "Ubuntu example: sudo apt install libbenchmark-dev")
    endif()
else()
    message(STATUS "Google Benchmark: disabled")
endif()