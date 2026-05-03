# Sanitizer settings for FPGA-Gated Execution Platform. (Only for debug builds)
# Usage from CMake:
#
#  include(cmake/Sanitizers.cmake)
#  fgep_enable_sanitizers(my_target)

option(FGEP_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(FGEP_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

function(fgep_enable_sanitizers target_name)
    if(MSVC)
        if(FGEP_ENABLE_ASAN)
            target_compile_options(${target_name} PRIVATE /fsanitize=address)
            target_link_options(${target_name} PRIVATE /fsanitize=address)
        endif()

        if(FGEP_ENABLE_UBSAN)
            message(WARNING "UndefinedBehaviorSanitizer is not supported by this MSVC configuration.")
        endif()

        return()
    endif()

    if(FGEP_ENABLE_ASAN)
        target_compile_options(
            ${target_name}
            PRIVATE
                -fsanitize=address
                -fno-omit-frame-pointer
        )

        target_link_options(
            ${target_name}
            PRIVATE
                -fsanitize=address
        )
    endif()

    if(FGEP_ENABLE_UBSAN)
        target_compile_options(
            ${target_name}
            PRIVATE
                -fsanitize=undefined
                -fno-omit-frame-pointer
        )

        target_link_options(
            ${target_name}
            PRIVATE
                -fsanitize=undefined
        )
    endif()
endfunction()