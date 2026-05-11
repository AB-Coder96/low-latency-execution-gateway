#include "fgep/telemetry/pipeline_telemetry.hpp"

#include <cassert>
#include <cstring>

int main() {
    using namespace fgep::telemetry;

    {
        PipelineTelemetryEvent event{};
        event.user_ref_num = 42;

        assert(event.user_ref_num == 42);
        assert(!event.has(PipelineStage::ingest));
        assert(!event.timestamp(PipelineStage::ingest).has_value());
        assert(!event.end_to_end_latency().has_value());
    }

    {
        PipelineTelemetryEvent event{};
        event.user_ref_num = 100;

        event.mark(PipelineStage::ingest, 1'000);
        event.mark(PipelineStage::decode, 1'025);
        event.mark(PipelineStage::book_update, 1'080);
        event.mark(PipelineStage::risk_decision, 1'120);
        event.mark(PipelineStage::lifecycle_decision, 1'150);
        event.mark(PipelineStage::enqueue, 1'190);
        event.mark(PipelineStage::submit, 1'250);

        assert(event.has(PipelineStage::ingest));
        assert(event.has(PipelineStage::submit));

        const auto decode_latency = event.latency(
            PipelineStage::ingest,
            PipelineStage::decode
        );
        assert(decode_latency.has_value());
        assert(decode_latency.value() == 25);

        const auto risk_to_lifecycle = event.latency(
            PipelineStage::risk_decision,
            PipelineStage::lifecycle_decision
        );
        assert(risk_to_lifecycle.has_value());
        assert(risk_to_lifecycle.value() == 30);

        const auto end_to_end = event.end_to_end_latency();
        assert(end_to_end.has_value());
        assert(end_to_end.value() == 250);
    }

    {
        PipelineTelemetryEvent event{};

        event.mark(PipelineStage::decode, 2'000);
        event.mark(PipelineStage::submit, 2'100);

        const auto missing = event.end_to_end_latency();

        assert(!missing.has_value());
    }

    {
        PipelineTelemetryEvent event{};

        event.mark(PipelineStage::ingest, 2'000);
        event.mark(PipelineStage::submit, 1'900);

        const auto reversed = event.end_to_end_latency();

        assert(!reversed.has_value());
    }

    {
        PipelineTelemetryEvent event{};
        event.user_ref_num = 77;

        event.mark(PipelineStage::ingest, 10);
        event.mark(PipelineStage::submit, 20);

        assert(event.has(PipelineStage::ingest));
        assert(event.user_ref_num == 77);

        event.clear();

        assert(event.user_ref_num == 0);
        assert(!event.has(PipelineStage::ingest));
        assert(!event.has(PipelineStage::submit));
        assert(!event.end_to_end_latency().has_value());
    }

    {
        assert(std::strcmp(stage_name(PipelineStage::ingest), "ingest") == 0);
        assert(std::strcmp(stage_name(PipelineStage::decode), "decode") == 0);
        assert(
            std::strcmp(
                stage_name(PipelineStage::book_update),
                "book_update"
            ) == 0
        );
        assert(
            std::strcmp(
                stage_name(PipelineStage::risk_decision),
                "risk_decision"
            ) == 0
        );
        assert(
            std::strcmp(
                stage_name(PipelineStage::lifecycle_decision),
                "lifecycle_decision"
            ) == 0
        );
        assert(std::strcmp(stage_name(PipelineStage::enqueue), "enqueue") == 0);
        assert(std::strcmp(stage_name(PipelineStage::submit), "submit") == 0);
        assert(std::strcmp(stage_name(PipelineStage::count), "count") == 0);
    }

    return 0;
}