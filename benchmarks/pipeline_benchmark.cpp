#include "hotpath_benchmark.hpp"

#include "fgep/telemetry/latency_summary.hpp"
#include "fgep/telemetry/pipeline_telemetry.hpp"

#include <cstdint>

int main() {
    constexpr std::uint64_t iterations = 10'000'000U;

    fgep::telemetry::LatencyRecorder recorder{};

    const auto result = fgep::benchmarks::run_hot_loop(
        "hotpath_pipeline_telemetry_event",
        iterations,
        [&](std::uint64_t index) {
            fgep::telemetry::PipelineTelemetryEvent event{};

            event.user_ref_num =
                static_cast<fgep::ouch::UserRefNum>((index % 1'000'000U) + 1U);

            const auto base = static_cast<fgep::telemetry::TimestampNs>(
                index * 100U
            );

            event.mark(fgep::telemetry::PipelineStage::ingest, base);
            event.mark(fgep::telemetry::PipelineStage::decode, base + 10U);
            event.mark(fgep::telemetry::PipelineStage::book_update, base + 20U);
            event.mark(
                fgep::telemetry::PipelineStage::lifecycle_decision,
                base + 30U
            );
            event.mark(fgep::telemetry::PipelineStage::enqueue, base + 40U);
            event.mark(fgep::telemetry::PipelineStage::submit, base + 50U);

            const auto latency = event.end_to_end_latency();

            if (latency.has_value()) {
                recorder.record(latency.value());
            }

            fgep::benchmarks::do_not_optimize(event);
            fgep::benchmarks::do_not_optimize(latency);
            fgep::benchmarks::clobber_memory();

            if (recorder.size() >= 4096U) {
                recorder.clear();
            }
        }
    );

    fgep::benchmarks::print_result(result);
    return 0;
}