#pragma once

#include "fgep/telemetry/pipeline_telemetry.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace fgep::telemetry {

struct LatencySummary {
    std::size_t count{};
    DurationNs min_ns{};
    DurationNs max_ns{};
    DurationNs mean_ns{};
    DurationNs p50_ns{};
    DurationNs p90_ns{};
    DurationNs p99_ns{};
    DurationNs p999_ns{};
};

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