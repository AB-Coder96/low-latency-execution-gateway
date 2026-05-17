#pragma once

#include "fgep/execution/backend_factory.hpp"
#include "fgep/telemetry/latency_summary.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace fgep::bench {

struct BackendBenchmarkConfig {
    std::vector<execution::BackendTechnology> technologies{
        execution::BackendTechnology::recording,
        execution::BackendTechnology::kernel_udp
    };

    std::size_t submission_count{1'000};
    std::size_t payload_size{64};

    telemetry::DurationNs base_submit_latency_ns{1'000};
    telemetry::DurationNs latency_jitter_step_ns{25};
    telemetry::DurationNs latency_jitter_period{11};

    execution::KernelUdpBackendConfig kernel_udp{};
    execution::AfXdpBackendConfig afxdp{};
    execution::DpdkBackendConfig dpdk{};
};

struct BackendBenchmarkTechnologyResult {
    execution::BackendTechnology technology{
        execution::BackendTechnology::recording
    };

    std::size_t submission_count{};
    std::size_t accepted_count{};
    std::size_t rejected_count{};
    telemetry::DurationNs simulated_elapsed_ns{};
    std::uint64_t submissions_per_second{};
    std::optional<telemetry::LatencySummary> submit_latency{};
};

struct BackendBenchmarkResult {
    std::vector<BackendBenchmarkTechnologyResult> technologies{};
};

[[nodiscard]] BackendBenchmarkResult run_backend_benchmark(
    const BackendBenchmarkConfig& config
);

[[nodiscard]] BackendBenchmarkTechnologyResult run_backend_benchmark_for(
    const BackendBenchmarkConfig& config,
    execution::BackendTechnology technology
);

} // namespace fgep::bench