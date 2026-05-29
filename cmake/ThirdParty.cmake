# Optional third-party dependency discovery for FGEP.
#
# This file intentionally does not download dependencies.
# Dependencies such as GoogleTest should be installed by the developer,
# package manager, or CI image.

set(FGEP_GOOGLETEST_AVAILABLE OFF)

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
        message(WARNING "Install GoogleTest development files, then reconfigure.")
        message(WARNING "Ubuntu example: sudo apt install libgtest-dev")
    endif()
else()
    message(STATUS "GoogleTest: disabled")
endif()