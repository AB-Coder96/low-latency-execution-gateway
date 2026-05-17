#pragma once

#include "fgep/gate/guardrail_control.hpp"
#include "fgep/telemetry/latency_summary.hpp"

#include <cstddef>
#include <optional>

namespace fgep::gate {

struct GuardrailUpdateBenchmarkConfig {
    std::size_t update_count{1'000};
    telemetry::DurationNs base_update_latency_ns{1'000};
    telemetry::DurationNs latency_jitter_step_ns{25};
    telemetry::DurationNs latency_jitter_period{7};
};

struct GuardrailUpdateBenchmarkResult {
    std::size_t update_count{};
    std::size_t applied_count{};
    std::size_t dropped_count{};
    telemetry::DurationNs simulated_elapsed_ns{};
    std::optional<telemetry::LatencySummary> update_latency{};
};

[[nodiscard]] GuardrailUpdateBenchmarkResult run_guardrail_update_benchmark(
    const GuardrailUpdateBenchmarkConfig& config
);

} // namespace fgep::gate