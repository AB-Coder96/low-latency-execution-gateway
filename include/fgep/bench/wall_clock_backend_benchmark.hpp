#pragma once

#include "fgep/execution/backend_factory.hpp"
#include "fgep/telemetry/latency_summary.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace fgep::bench {

struct WallClockBackendBenchmarkConfig {
    std::vector<execution::BackendTechnology> technologies{
        execution::BackendTechnology::recording,
        execution::BackendTechnology::kernel_udp
    };

    std::size_t submission_count{100'000};
    std::size_t payload_size{64};

    bool record_per_submit_latency{true};

    execution::KernelUdpBackendConfig kernel_udp{};
    execution::AfXdpBackendConfig afxdp{};
    execution::DpdkBackendConfig dpdk{};
};

struct WallClockBackendTechnologyResult {
    execution::BackendTechnology technology{
        execution::BackendTechnology::recording
    };

    std::size_t submission_count{};
    std::size_t accepted_count{};
    std::size_t rejected_count{};

    telemetry::DurationNs elapsed_ns{};
    std::uint64_t submissions_per_second{};

    std::optional<telemetry::LatencySummary> submit_latency{};
};

struct WallClockBackendBenchmarkResult {
    std::vector<WallClockBackendTechnologyResult> technologies{};
};

[[nodiscard]] WallClockBackendBenchmarkResult run_wall_clock_backend_benchmark(
    const WallClockBackendBenchmarkConfig& config
);

[[nodiscard]] WallClockBackendTechnologyResult
run_wall_clock_backend_benchmark_for(
    const WallClockBackendBenchmarkConfig& config,
    execution::BackendTechnology technology
);

} // namespace fgep::bench