#include "fgep/telemetry/latency_summary.hpp"

#include <algorithm>
#include <limits>

namespace fgep::telemetry {
namespace {

[[nodiscard]] DurationNs percentile_nearest_rank(
    const std::vector<DurationNs>& sorted_samples,
    std::size_t numerator,
    std::size_t denominator
) noexcept {
    if (sorted_samples.empty() || denominator == 0) {
        return 0;
    }

    const auto count = sorted_samples.size();
    const auto rank = ((count * numerator) + denominator - 1U) / denominator;
    const auto raw_index = rank == 0 ? 0 : rank - 1U;
    const auto index = raw_index >= count ? count - 1U : raw_index;

    return sorted_samples[index];
}

[[nodiscard]] DurationNs saturating_add(
    DurationNs left,
    DurationNs right
) noexcept {
    if (right > std::numeric_limits<DurationNs>::max() - left) {
        return std::numeric_limits<DurationNs>::max();
    }

    return left + right;
}

} // namespace

std::optional<LatencySummary> summarize_latencies(
    std::vector<DurationNs> samples
) {
    if (samples.empty()) {
        return std::nullopt;
    }

    std::sort(samples.begin(), samples.end());

    DurationNs total = 0;

    for (const auto sample : samples) {
        total = saturating_add(total, sample);
    }

    const auto count = samples.size();

    return LatencySummary{
        .count = count,
        .min_ns = samples.front(),
        .max_ns = samples.back(),
        .mean_ns = total / static_cast<DurationNs>(count),
        .p50_ns = percentile_nearest_rank(samples, 50, 100),
        .p90_ns = percentile_nearest_rank(samples, 90, 100),
        .p99_ns = percentile_nearest_rank(samples, 99, 100),
        .p999_ns = percentile_nearest_rank(samples, 999, 1000)
    };
}

void LatencyRecorder::record(DurationNs latency_ns) {
    samples_.push_back(latency_ns);
}

bool LatencyRecorder::record_end_to_end(
    const PipelineTelemetryEvent& event
) {
    const auto latency = event.end_to_end_latency();

    if (!latency.has_value()) {
        return false;
    }

    record(latency.value());
    return true;
}

std::optional<LatencySummary> LatencyRecorder::summary() const {
    return summarize_latencies(samples_);
}

bool LatencyRecorder::empty() const noexcept {
    return samples_.empty();
}

std::size_t LatencyRecorder::size() const noexcept {
    return samples_.size();
}

void LatencyRecorder::clear() noexcept {
    samples_.clear();
}

} // namespace fgep::telemetry