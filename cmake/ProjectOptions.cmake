# Project-wide CMake options for FPGA-Gated Execution Platform.
option(FGEP_BUILD_TESTS "Build unit and integration tests" ON)
option(FGEP_BUILD_APPS "Build demo applications" ON)
option(FGEP_BUILD_BENCHMARKS "Build benchmark executables" OFF)

option(FGEP_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(FGEP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

option(FGEP_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(FGEP_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

option(FGEP_ENABLE_KERNEL_UDP "Build kernel UDP execution backend" ON)
option(FGEP_ENABLE_AF_XDP "Build AF_XDP execution backend" OFF)
option(FGEP_ENABLE_HARDWARE_GATE "Build simulated hardware gate components" ON)
option(FGEP_ENABLE_CORUNDUM "Build Corundum control-path components" OFF)

function(fgep_print_project_options)
    message(STATUS "FGEP_BUILD_TESTS: ${FGEP_BUILD_TESTS}")
    message(STATUS "FGEP_BUILD_APPS: ${FGEP_BUILD_APPS}")
    message(STATUS "FGEP_BUILD_BENCHMARKS: ${FGEP_BUILD_BENCHMARKS}")

    message(STATUS "FGEP_ENABLE_WARNINGS: ${FGEP_ENABLE_WARNINGS}")
    message(STATUS "FGEP_WARNINGS_AS_ERRORS: ${FGEP_WARNINGS_AS_ERRORS}")

    message(STATUS "FGEP_ENABLE_ASAN: ${FGEP_ENABLE_ASAN}")
    message(STATUS "FGEP_ENABLE_UBSAN: ${FGEP_ENABLE_UBSAN}")

    message(STATUS "FGEP_ENABLE_KERNEL_UDP: ${FGEP_ENABLE_KERNEL_UDP}")
    message(STATUS "FGEP_ENABLE_AF_XDP: ${FGEP_ENABLE_AF_XDP}")
    message(STATUS "FGEP_ENABLE_HARDWARE_GATE: ${FGEP_ENABLE_HARDWARE_GATE}")
    message(STATUS "FGEP_ENABLE_CORUNDUM: ${FGEP_ENABLE_CORUNDUM}")
endfunction()