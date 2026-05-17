#pragma once

#include "fgep/bench/order_candidate_generator.hpp"
#include "fgep/risk/risk_supervisor.hpp"
#include "fgep/telemetry/latency_summary.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace fgep::bench {

struct BenchmarkStageDurations {
    telemetry::DurationNs ingest_to_decode_ns{25};
    telemetry::DurationNs decode_to_book_update_ns{35};
    telemetry::DurationNs book_update_to_risk_ns{20};
    telemetry::DurationNs risk_to_lifecycle_ns{15};
    telemetry::DurationNs lifecycle_to_enqueue_ns{10};
    telemetry::DurationNs enqueue_to_submit_ns{25};
    telemetry::DurationNs candidate_spacing_ns{1'000};
};

struct ExecutionBenchmarkConfig {
    std::size_t candidate_count{1'000};
    OrderCandidateGeneratorConfig generator_config{};
    risk::RiskLimits risk_limits{};
    BenchmarkStageDurations stage_durations{};
};

struct ExecutionBenchmarkResult {
    std::size_t candidate_count{};
    std::size_t accepted_count{};
    std::size_t rejected_count{};
    std::size_t live_order_count{};
    telemetry::DurationNs simulated_elapsed_ns{};
    std::uint64_t candidates_per_second{};
    std::optional<telemetry::LatencySummary> end_to_end_latency{};
};

[[nodiscard]] ExecutionBenchmarkResult run_execution_benchmark(
    const ExecutionBenchmarkConfig& config
);

} // namespace fgep::bench