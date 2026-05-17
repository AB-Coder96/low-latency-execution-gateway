#include "fgep/bench/execution_benchmark.hpp"

#include "fgep/execution/simulated_execution_engine.hpp"

#include <limits>

namespace fgep::bench {
namespace {

[[nodiscard]] telemetry::DurationNs per_candidate_latency(
    const BenchmarkStageDurations& durations
) noexcept {
    return durations.ingest_to_decode_ns
        + durations.decode_to_book_update_ns
        + durations.book_update_to_risk_ns
        + durations.risk_to_lifecycle_ns
        + durations.lifecycle_to_enqueue_ns
        + durations.enqueue_to_submit_ns;
}

[[nodiscard]] telemetry::TimestampNs candidate_base_timestamp(
    std::size_t index,
    telemetry::DurationNs candidate_spacing_ns
) noexcept {
    const auto safe_index = static_cast<telemetry::TimestampNs>(index);

    if (
        candidate_spacing_ns != 0
        && safe_index
            > std::numeric_limits<telemetry::TimestampNs>::max()
                / candidate_spacing_ns
    ) {
        return std::numeric_limits<telemetry::TimestampNs>::max();
    }

    return safe_index * candidate_spacing_ns;
}

[[nodiscard]] std::uint64_t calculate_candidates_per_second(
    std::size_t candidate_count,
    telemetry::DurationNs elapsed_ns
) noexcept {
    if (candidate_count == 0 || elapsed_ns == 0) {
        return 0;
    }

    const auto count = static_cast<std::uint64_t>(candidate_count);

    if (count > std::numeric_limits<std::uint64_t>::max() / 1'000'000'000ULL) {
        return std::numeric_limits<std::uint64_t>::max();
    }

    return (count * 1'000'000'000ULL) / elapsed_ns;
}

void mark_pipeline_timestamps(
    telemetry::PipelineTelemetryEvent& event,
    telemetry::TimestampNs base,
    const BenchmarkStageDurations& durations
) noexcept {
    auto timestamp = base;

    event.mark(telemetry::PipelineStage::ingest, timestamp);

    timestamp += durations.ingest_to_decode_ns;
    event.mark(telemetry::PipelineStage::decode, timestamp);

    timestamp += durations.decode_to_book_update_ns;
    event.mark(telemetry::PipelineStage::book_update, timestamp);

    timestamp += durations.book_update_to_risk_ns;
    event.mark(telemetry::PipelineStage::risk_decision, timestamp);

    timestamp += durations.risk_to_lifecycle_ns;
    event.mark(telemetry::PipelineStage::lifecycle_decision, timestamp);

    timestamp += durations.lifecycle_to_enqueue_ns;
    event.mark(telemetry::PipelineStage::enqueue, timestamp);

    timestamp += durations.enqueue_to_submit_ns;
    event.mark(telemetry::PipelineStage::submit, timestamp);
}

} // namespace

ExecutionBenchmarkResult run_execution_benchmark(
    const ExecutionBenchmarkConfig& config
) {
    OrderCandidateGenerator generator{config.generator_config};
    execution::SimulatedExecutionEngine engine{
        risk::RiskSupervisor{config.risk_limits}
    };
    telemetry::LatencyRecorder latency_recorder{};

    ExecutionBenchmarkResult result{
        .candidate_count = config.candidate_count,
        .accepted_count = 0,
        .rejected_count = 0,
        .live_order_count = 0,
        .simulated_elapsed_ns = 0,
        .candidates_per_second = 0,
        .end_to_end_latency = std::nullopt
    };

    for (std::size_t index = 0; index < config.candidate_count; ++index) {
        const auto message = generator.next_enter_order();
        const auto execution_result = engine.submit_enter(message);

        if (execution_result.accepted()) {
            ++result.accepted_count;
        } else {
            ++result.rejected_count;
        }

        telemetry::PipelineTelemetryEvent event{};
        event.user_ref_num = message.user_ref_num;

        mark_pipeline_timestamps(
            event,
            candidate_base_timestamp(
                index,
                config.stage_durations.candidate_spacing_ns
            ),
            config.stage_durations
        );

        latency_recorder.record_end_to_end(event);
    }

    result.live_order_count = engine.live_order_count();

    if (config.candidate_count != 0) {
        const auto last_base = candidate_base_timestamp(
            config.candidate_count - 1U,
            config.stage_durations.candidate_spacing_ns
        );

        result.simulated_elapsed_ns = last_base
            + per_candidate_latency(config.stage_durations);
    }

    result.candidates_per_second = calculate_candidates_per_second(
        result.candidate_count,
        result.simulated_elapsed_ns
    );
    result.end_to_end_latency = latency_recorder.summary();

    return result;
}

} // namespace fgep::bench