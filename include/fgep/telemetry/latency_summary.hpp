#pragma once

#include "fgep/telemetry/pipeline_telemetry.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace fgep::telemetry {

enum class LatencyMeasurementKind {
    synthetic_deterministic,
    wall_clock_host,
    network_end_to_end
};

struct LatencySummary {
    std::size_t count{};
    DurationNs min_ns{};
    DurationNs max_ns{};
    DurationNs mean_ns{};
    DurationNs p50_ns{};
    DurationNs p90_ns{};
    DurationNs p99_ns{};
    DurationNs p999_ns{};
    std::size_t warmup_count{};
};

[[nodiscard]] std::string_view latency_measurement_kind_name(
    LatencyMeasurementKind kind
) noexcept;

[[nodiscard]] std::string_view latency_measurement_kind_note(
    LatencyMeasurementKind kind
) noexcept;

[[nodiscard]] std::optional<LatencySummary> summarize_latencies(
    std::vector<DurationNs> samples
);

class LatencyRecorder {
public:
    void record(DurationNs latency_ns);

    [[nodiscard]] bool record_end_to_end(
        const PipelineTelemetryEvent& event
    );

    [[nodiscard]] std::optional<LatencySummary> summary() const;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    void clear() noexcept;

private:
    std::vector<DurationNs> samples_{};
};

} // namespace fgep::telemetry