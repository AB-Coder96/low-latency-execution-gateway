#include "fgep/telemetry/latency_summary.hpp"

#include <gtest/gtest.h>

namespace {

TEST(LatencySummary, EmptySamplesReturnNullopt) {
    const auto summary = fgep::telemetry::summarize_latencies({});

    EXPECT_FALSE(summary.has_value());
}

TEST(LatencySummary, SingleSampleSetsEveryPercentile) {
    const auto summary = fgep::telemetry::summarize_latencies({42});

    ASSERT_TRUE(summary.has_value());
    EXPECT_EQ(summary->count, 1U);
    EXPECT_EQ(summary->min_ns, 42U);
    EXPECT_EQ(summary->max_ns, 42U);
    EXPECT_EQ(summary->mean_ns, 42U);
    EXPECT_EQ(summary->p50_ns, 42U);
    EXPECT_EQ(summary->p90_ns, 42U);
    EXPECT_EQ(summary->p99_ns, 42U);
    EXPECT_EQ(summary->p999_ns, 42U);
    EXPECT_EQ(summary->warmup_count, 0U);
}

TEST(LatencySummary, SortsSamplesBeforeComputingPercentiles) {
    const auto summary = fgep::telemetry::summarize_latencies({
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

    ASSERT_TRUE(summary.has_value());
    EXPECT_EQ(summary->count, 10U);
    EXPECT_EQ(summary->min_ns, 10U);
    EXPECT_EQ(summary->max_ns, 100U);
    EXPECT_EQ(summary->mean_ns, 55U);
    EXPECT_EQ(summary->p50_ns, 50U);
    EXPECT_EQ(summary->p90_ns, 90U);
    EXPECT_EQ(summary->p99_ns, 100U);
    EXPECT_EQ(summary->p999_ns, 100U);
}

TEST(LatencySummary, NearestRankPercentilesRoundUp) {
    const auto summary = fgep::telemetry::summarize_latencies({
        1,
        2,
        3,
        4,
        5
    });

    ASSERT_TRUE(summary.has_value());
    EXPECT_EQ(summary->count, 5U);
    EXPECT_EQ(summary->min_ns, 1U);
    EXPECT_EQ(summary->max_ns, 5U);
    EXPECT_EQ(summary->mean_ns, 3U);
    EXPECT_EQ(summary->p50_ns, 3U);
    EXPECT_EQ(summary->p90_ns, 5U);
    EXPECT_EQ(summary->p99_ns, 5U);
    EXPECT_EQ(summary->p999_ns, 5U);
}

TEST(LatencyRecorder, RecordsSummarizesAndClearsSamples) {
    fgep::telemetry::LatencyRecorder recorder{};

    EXPECT_TRUE(recorder.empty());
    EXPECT_EQ(recorder.size(), 0U);
    EXPECT_FALSE(recorder.summary().has_value());

    recorder.record(100);
    recorder.record(300);
    recorder.record(200);

    EXPECT_FALSE(recorder.empty());
    EXPECT_EQ(recorder.size(), 3U);

    const auto summary = recorder.summary();

    ASSERT_TRUE(summary.has_value());
    EXPECT_EQ(summary->count, 3U);
    EXPECT_EQ(summary->min_ns, 100U);
    EXPECT_EQ(summary->max_ns, 300U);
    EXPECT_EQ(summary->mean_ns, 200U);
    EXPECT_EQ(summary->p50_ns, 200U);

    recorder.clear();

    EXPECT_TRUE(recorder.empty());
    EXPECT_EQ(recorder.size(), 0U);
    EXPECT_FALSE(recorder.summary().has_value());
}

TEST(LatencyRecorder, RecordsValidEndToEndTelemetryEvents) {
    fgep::telemetry::PipelineTelemetryEvent event{};
    fgep::telemetry::LatencyRecorder recorder{};

    EXPECT_FALSE(recorder.record_end_to_end(event));
    EXPECT_TRUE(recorder.empty());

    event.mark(fgep::telemetry::PipelineStage::ingest, 1'000);
    event.mark(fgep::telemetry::PipelineStage::submit, 1'275);

    EXPECT_TRUE(recorder.record_end_to_end(event));
    EXPECT_EQ(recorder.size(), 1U);

    const auto summary = recorder.summary();

    ASSERT_TRUE(summary.has_value());
    EXPECT_EQ(summary->count, 1U);
    EXPECT_EQ(summary->min_ns, 275U);
    EXPECT_EQ(summary->max_ns, 275U);
    EXPECT_EQ(summary->mean_ns, 275U);
}

TEST(LatencyRecorder, RejectsReversedEndToEndTelemetryEvents) {
    fgep::telemetry::PipelineTelemetryEvent reversed{};
    fgep::telemetry::LatencyRecorder recorder{};

    reversed.mark(fgep::telemetry::PipelineStage::ingest, 2'000);
    reversed.mark(fgep::telemetry::PipelineStage::submit, 1'000);

    EXPECT_FALSE(recorder.record_end_to_end(reversed));
    EXPECT_TRUE(recorder.empty());
}

TEST(LatencyMeasurementKind, NamesAreHumanReadable) {
    EXPECT_EQ(
        fgep::telemetry::latency_measurement_kind_name(
            fgep::telemetry::LatencyMeasurementKind::synthetic_deterministic
        ),
        "synthetic deterministic latency"
    );

    EXPECT_EQ(
        fgep::telemetry::latency_measurement_kind_name(
            fgep::telemetry::LatencyMeasurementKind::wall_clock_host
        ),
        "wall-clock host latency"
    );

    EXPECT_EQ(
        fgep::telemetry::latency_measurement_kind_name(
            fgep::telemetry::LatencyMeasurementKind::network_end_to_end
        ),
        "network end-to-end latency"
    );
}

TEST(LatencyMeasurementKind, NotesDescribeMeasurementLimitations) {
    EXPECT_NE(
        fgep::telemetry::latency_measurement_kind_note(
            fgep::telemetry::LatencyMeasurementKind::synthetic_deterministic
        ).find("not real elapsed"),
        std::string_view::npos
    );

    EXPECT_NE(
        fgep::telemetry::latency_measurement_kind_note(
            fgep::telemetry::LatencyMeasurementKind::wall_clock_host
        ).find("clock-read overhead"),
        std::string_view::npos
    );
}

} // namespace