#include "fgep/telemetry/latency_summary.hpp"

#include <cassert>
#include <vector>

int main() {
    using namespace fgep::telemetry;

    {
        const auto summary = summarize_latencies({});

        assert(!summary.has_value());
    }

    {
        const auto summary = summarize_latencies({42});

        assert(summary.has_value());
        assert(summary->count == 1);
        assert(summary->min_ns == 42);
        assert(summary->max_ns == 42);
        assert(summary->mean_ns == 42);
        assert(summary->p50_ns == 42);
        assert(summary->p90_ns == 42);
        assert(summary->p99_ns == 42);
        assert(summary->p999_ns == 42);
    }

    {
        const auto summary = summarize_latencies({
            100,
            10,
            90,
            20,
            80,
            30,
            70,
            40,
            60,
            50
        });

        assert(summary.has_value());
        assert(summary->count == 10);
        assert(summary->min_ns == 10);
        assert(summary->max_ns == 100);
        assert(summary->mean_ns == 55);
        assert(summary->p50_ns == 50);
        assert(summary->p90_ns == 90);
        assert(summary->p99_ns == 100);
        assert(summary->p999_ns == 100);
    }

    {
        const auto summary = summarize_latencies({
            1,
            2,
            3,
            4,
            5
        });

        assert(summary.has_value());
        assert(summary->count == 5);
        assert(summary->min_ns == 1);
        assert(summary->max_ns == 5);
        assert(summary->mean_ns == 3);
        assert(summary->p50_ns == 3);
        assert(summary->p90_ns == 5);
        assert(summary->p99_ns == 5);
        assert(summary->p999_ns == 5);
    }

    {
        LatencyRecorder recorder{};

        assert(recorder.empty());
        assert(recorder.size() == 0);
        assert(!recorder.summary().has_value());

        recorder.record(100);
        recorder.record(300);
        recorder.record(200);

        assert(!recorder.empty());
        assert(recorder.size() == 3);

        const auto summary = recorder.summary();

        assert(summary.has_value());
        assert(summary->count == 3);
        assert(summary->min_ns == 100);
        assert(summary->max_ns == 300);
        assert(summary->mean_ns == 200);
        assert(summary->p50_ns == 200);

        recorder.clear();

        assert(recorder.empty());
        assert(recorder.size() == 0);
        assert(!recorder.summary().has_value());
    }

    {
        PipelineTelemetryEvent event{};
        LatencyRecorder recorder{};

        assert(!recorder.record_end_to_end(event));
        assert(recorder.empty());

        event.mark(PipelineStage::ingest, 1'000);
        event.mark(PipelineStage::submit, 1'275);

        assert(recorder.record_end_to_end(event));
        assert(recorder.size() == 1);

        const auto summary = recorder.summary();

        assert(summary.has_value());
        assert(summary->count == 1);
        assert(summary->min_ns == 275);
        assert(summary->max_ns == 275);
        assert(summary->mean_ns == 275);
    }

    {
        PipelineTelemetryEvent reversed{};
        LatencyRecorder recorder{};

        reversed.mark(PipelineStage::ingest, 2'000);
        reversed.mark(PipelineStage::submit, 1'000);

        assert(!recorder.record_end_to_end(reversed));
        assert(recorder.empty());
    }

    return 0;
}