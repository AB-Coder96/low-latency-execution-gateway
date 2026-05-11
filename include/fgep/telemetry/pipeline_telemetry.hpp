#pragma once

#include "fgep/ouch/ouch_types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace fgep::telemetry {

using TimestampNs = std::uint64_t;
using DurationNs = std::uint64_t;

enum class PipelineStage : std::uint8_t {
    ingest,
    decode,
    book_update,
    risk_decision,
    lifecycle_decision,
    enqueue,
    submit,
    count
};

inline constexpr std::size_t pipeline_stage_count{
    static_cast<std::size_t>(PipelineStage::count)
};

struct StageLatency {
    PipelineStage from{PipelineStage::ingest};
    PipelineStage to{PipelineStage::ingest};
    DurationNs duration_ns{};
};

struct PipelineTelemetryEvent {
    ouch::UserRefNum user_ref_num{};
    std::array<TimestampNs, pipeline_stage_count> timestamps{};
    std::array<bool, pipeline_stage_count> observed{};

    void mark(PipelineStage stage, TimestampNs timestamp_ns) noexcept;

    [[nodiscard]] bool has(PipelineStage stage) const noexcept;

    [[nodiscard]] std::optional<TimestampNs> timestamp(
        PipelineStage stage
    ) const noexcept;

    [[nodiscard]] std::optional<DurationNs> latency(
        PipelineStage from,
        PipelineStage to
    ) const noexcept;

    [[nodiscard]] std::optional<DurationNs> end_to_end_latency()
        const noexcept;

    void clear() noexcept;
};

[[nodiscard]] constexpr std::size_t stage_index(PipelineStage stage) noexcept {
    return static_cast<std::size_t>(stage);
}

[[nodiscard]] const char* stage_name(PipelineStage stage) noexcept;

} // namespace fgep::telemetry