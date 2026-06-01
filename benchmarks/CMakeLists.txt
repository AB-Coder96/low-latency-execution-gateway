# Google Benchmark targets for FGEP runtime and hot-path components.

message(STATUS "Configuring FGEP Google Benchmark targets")

if(NOT FGEP_GOOGLE_BENCHMARK_AVAILABLE)
    message(STATUS "Skipping Google Benchmark targets because Google Benchmark is unavailable.")
    return()
endif()

function(fgep_add_google_benchmark benchmark_name source_file)
    add_executable(
        ${benchmark_name}
        ${source_file}
    )

    target_link_libraries(
        ${benchmark_name}
        PRIVATE
            fgep
            benchmark::benchmark_main
    )

    target_compile_features(
        ${benchmark_name}
        PRIVATE
            cxx_std_20
    )
endfunction()

fgep_add_google_benchmark(
    order_lifecycle_benchmark
    order_lifecycle_benchmark.cpp
)

fgep_add_google_benchmark(
    symbol_order_book_benchmark
    symbol_order_book_benchmark.cpp
)

fgep_add_google_benchmark(
    encoded_submit_benchmark
    encoded_submit_benchmark.cpp
)

fgep_add_google_benchmark(
    instrument_directory_benchmark
    instrument_directory_benchmark.cpp
)