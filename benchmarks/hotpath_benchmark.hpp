#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace fgep::benchmarks {

template <typename T>
inline void do_not_optimize(const T& value) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "g"(&value) : "memory");
#else
    (void)value;
#endif
}

inline void clobber_memory() noexcept {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : : "memory");
#endif
}

struct HotPathResult {
    std::string_view name{};
    std::uint64_t iterations{};
    std::uint64_t elapsed_ns{};
    double operations_per_second{};
};

template <typename Function>
[[nodiscard]] HotPathResult run_hot_loop(
    std::string_view name,
    std::uint64_t iterations,
    Function&& function
) {
    for (std::uint64_t index = 0; index < 1024U; ++index) {
        function(index);
    }

    const auto start = std::chrono::steady_clock::now();

    for (std::uint64_t index = 0; index < iterations; ++index) {
        function(index);
    }

    const auto stop = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        stop - start
    ).count();

    const auto elapsed_ns = elapsed > 0 ? static_cast<std::uint64_t>(elapsed) : 1U;

    return HotPathResult{
        .name = name,
        .iterations = iterations,
        .elapsed_ns = elapsed_ns,
        .operations_per_second =
            static_cast<double>(iterations) * 1'000'000'000.0
                / static_cast<double>(elapsed_ns)
    };
}

inline void print_result(const HotPathResult& result) {
    std::cout << result.name << '\n';
    std::cout << "  iterations: " << result.iterations << '\n';
    std::cout << "  elapsed_ns: " << result.elapsed_ns << '\n';
    std::cout << "  ops_per_sec: " << result.operations_per_second << '\n';
}

} // namespace fgep::benchmarks