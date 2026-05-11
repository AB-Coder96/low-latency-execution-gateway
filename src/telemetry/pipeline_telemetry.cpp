#include "fgep/telemetry/pipeline_telemetry.hpp"

namespace fgep::telemetry {

void PipelineTelemetryEvent::mark(
    PipelineStage stage,
    TimestampNs timestamp_ns
) noexcept {
    if (stage == PipelineStage::count) {
        return;
    }

    const auto index = stage_index(stage);
    timestamps[index] = timestamp_ns;
    observed[index] = true;
}

bool PipelineTelemetryEvent::has(PipelineStage stage) const noexcept {
    if (stage == PipelineStage::count) {
        return false;
    }

    return observed[stage_index(stage)];
}

std::optional<TimestampNs> PipelineTelemetryEvent::timestamp(
    PipelineStage stage
) const noexcept {
    if (!has(stage)) {
        return std::nullopt;
    }

    return timestamps[stage_index(stage)];
}

std::optional<DurationNs> PipelineTelemetryEvent::latency(
    PipelineStage from,
    PipelineStage to
) const noexcept {
    const auto start = timestamp(from);
    const auto finish = timestamp(to);

    if (!start.has_value() || !finish.has_value()) {
        return std::nullopt;
    }

    if (finish.value() < start.value()) {
        return std::nullopt;
    }

    return finish.value() - start.value();
}

std::optional<DurationNs> PipelineTelemetryEvent::end_to_end_latency()
    const noexcept {
    return latency(PipelineStage::ingest, PipelineStage::submit);
}

void PipelineTelemetryEvent::clear() noexcept {
    timestamps.fill(0);
    observed.fill(false);
    user_ref_num = 0;
}

const char* stage_name(PipelineStage stage) noexcept {
    switch (stage) {
    case PipelineStage::ingest:
        return "ingest";
    case PipelineStage::decode:
        return "decode";
    case PipelineStage::book_update:
        return "book_update";
    case PipelineStage::risk_decision:
        return "risk_decision";
    case PipelineStage::lifecycle_decision:
        return "lifecycle_decision";
    case PipelineStage::enqueue:
        return "enqueue";
    case PipelineStage::submit:
        return "submit";
    case PipelineStage::count:
        return "count";
    }

    return "unknown";
}

} // namespace fgep::telemetry