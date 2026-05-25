#include "fgep/bench/benchmark_report.hpp"

#include <cassert>
#include <optional>

int main() {
    using namespace fgep::bench;

    {
        assert(format_duration_ns(42) == "42 ns");
        assert(format_rate_per_second(250000) == "250000 candidates/s");
    }

    {
        ExecutionBenchmarkResult result{
            .candidate_count = 5,
            .accepted_count = 4,
            .rejected_count = 1,
            .live_order_count = 4,
            .simulated_elapsed_ns = 4'210,
            .candidates_per_second = 1'187'648,
            .end_to_end_latency = fgep::telemetry::LatencySummary{
                .count = 5,
                .min_ns = 210,
                .max_ns = 210,
                .mean_ns = 210,
                .p50_ns = 210,
                .p90_ns = 210,
                .p99_ns = 210,
                .p999_ns = 210,
                .warmup_count = 0
            }
        };

        const auto report = format_execution_benchmark_markdown(
            result,
            BenchmarkReportMetadata{
                .title = "Deterministic Execution Benchmark",
                .backend_name = "simulated",
                .notes = "Synthetic deterministic timestamps.",
                .latency_measurement_kind =
                    fgep::telemetry::LatencyMeasurementKind::synthetic_deterministic,
                .warmup_count = 0
            }
        );

        assert(contains_text(report, "# Deterministic Execution Benchmark"));
        assert(contains_text(report, "Backend: `simulated`"));
        assert(contains_text(report, "Synthetic deterministic timestamps."));
        assert(contains_text(report, "| candidate_count | 5 |"));
        assert(contains_text(report, "| accepted_count | 4 |"));
        assert(contains_text(report, "| rejected_count | 1 |"));
        assert(contains_text(report, "| live_order_count | 4 |"));
        assert(contains_text(report, "| simulated_elapsed | 4210 ns |"));
        assert(contains_text(report, "| throughput | 1187648 candidates/s |"));
        assert(contains_text(report, "## End-to-end latency"));
        assert(contains_text(report, "| p99_ns | 210 |"));
        assert(contains_text(report, "| p999_ns | 210 |"));
        assert(contains_text(report, "## Measurement notes"));
        assert(contains_text(report, "| latency_type | synthetic deterministic latency |"));
        assert(contains_text(report, "| warmup_count | 0 |"));
        assert(contains_text(report, "| measurement_type | synthetic deterministic latency |"));
        assert(contains_text(report, "| measurement_notes | Synthetic deterministic samples"));
    }

    {
        ExecutionBenchmarkResult result{
            .candidate_count = 0,
            .accepted_count = 0,
            .rejected_count = 0,
            .live_order_count = 0,
            .simulated_elapsed_ns = 0,
            .candidates_per_second = 0,
            .end_to_end_latency = std::nullopt
        };

        const auto report = format_execution_benchmark_markdown(result);

        assert(contains_text(report, "# Execution Benchmark"));
        assert(contains_text(report, "Backend: `simulated`"));
        assert(contains_text(report, "No latency samples recorded."));
    }

    {
        assert(contains_text("abcdef", "bcd"));
        assert(!contains_text("abcdef", "xyz"));
    }

    {
        BackendBenchmarkResult result{
            .technologies = {
                BackendBenchmarkTechnologyResult{
                    .technology = fgep::execution::BackendTechnology::recording,
                    .submission_count = 3,
                    .accepted_count = 3,
                    .rejected_count = 0,
                    .simulated_elapsed_ns = 3'100,
                    .submissions_per_second = 967'741,
                    .submit_latency = fgep::telemetry::LatencySummary{
                        .count = 3,
                        .min_ns = 1'000,
                        .max_ns = 1'100,
                        .mean_ns = 1'033,
                        .p50_ns = 1'000,
                        .p90_ns = 1'100,
                        .p99_ns = 1'100,
                        .p999_ns = 1'100,
                        .warmup_count = 0
                    }
                },
                BackendBenchmarkTechnologyResult{
                    .technology = fgep::execution::BackendTechnology::afxdp,
                    .submission_count = 3,
                    .accepted_count = 0,
                    .rejected_count = 3,
                    .simulated_elapsed_ns = 3'100,
                    .submissions_per_second = 967'741,
                    .submit_latency = fgep::telemetry::LatencySummary{
                        .count = 3,
                        .min_ns = 1'000,
                        .max_ns = 1'100,
                        .mean_ns = 1'033,
                        .p50_ns = 1'000,
                        .p90_ns = 1'100,
                        .p99_ns = 1'100,
                        .p999_ns = 1'100,
                        .warmup_count = 0
                    }
                }
            }
        };

        const auto report = format_backend_benchmark_markdown(
            result,
            BenchmarkReportMetadata{
                .title = "Backend Comparison",
                .backend_name = "multi-backend",
                .notes = "Deterministic backend comparison.",
                .latency_measurement_kind =
                    fgep::telemetry::LatencyMeasurementKind::synthetic_deterministic,
                .warmup_count = 0
            }
        );

        assert(contains_text(report, "# Backend Comparison"));
        assert(contains_text(report, "Backend: `multi-backend`"));
        assert(contains_text(report, "Deterministic backend comparison."));
        assert(contains_text(report, "## Measurement notes"));
        assert(contains_text(report, "| latency_type | synthetic deterministic latency |"));
        assert(contains_text(report, "## Backend comparison"));
        assert(contains_text(report, "| recording | 3 | 3 | 0 | 3100 ns | 967741 candidates/s | 1000 | 1100 | 1100 |"));
        assert(contains_text(report, "| afxdp | 3 | 0 | 3 | 3100 ns | 967741 candidates/s | 1000 | 1100 | 1100 |"));
    }

    {
        BackendBenchmarkResult result{};

        const auto report = format_backend_benchmark_markdown(
            result,
            BenchmarkReportMetadata{
                .title = "Empty Backend Comparison",
                .backend_name = "none",
                .notes = {},
                .latency_measurement_kind =
                    fgep::telemetry::LatencyMeasurementKind::synthetic_deterministic,
                .warmup_count = 0
            }
        );

        assert(contains_text(report, "# Empty Backend Comparison"));
        assert(contains_text(report, "| n/a | 0 | 0 | 0 | 0 ns | 0 candidates/s | n/a | n/a | n/a |"));
    }

    {
        WallClockBackendBenchmarkResult result{
            .technologies = {
                WallClockBackendTechnologyResult{
                    .technology = fgep::execution::BackendTechnology::recording,
                    .submission_count = 100,
                    .accepted_count = 100,
                    .rejected_count = 0,
                    .elapsed_ns = 50'000,
                    .submissions_per_second = 2'000'000,
                    .submit_latency = fgep::telemetry::LatencySummary{
                        .count = 100,
                        .min_ns = 100,
                        .max_ns = 500,
                        .mean_ns = 250,
                        .p50_ns = 200,
                        .p90_ns = 400,
                        .p99_ns = 500,
                        .p999_ns = 500,
                        .warmup_count = 0
                    }
                },
                WallClockBackendTechnologyResult{
                    .technology = fgep::execution::BackendTechnology::kernel_udp,
                    .submission_count = 100,
                    .accepted_count = 100,
                    .rejected_count = 0,
                    .elapsed_ns = 100'000,
                    .submissions_per_second = 1'000'000,
                    .submit_latency = fgep::telemetry::LatencySummary{
                        .count = 100,
                        .min_ns = 200,
                        .max_ns = 900,
                        .mean_ns = 450,
                        .p50_ns = 400,
                        .p90_ns = 800,
                        .p99_ns = 900,
                        .p999_ns = 900,
                        .warmup_count = 0
                    }
                }
            }
        };

        const auto report = format_wall_clock_backend_benchmark_markdown(
            result,
            BenchmarkReportMetadata{
                .title = "Wall Clock Backend Benchmark",
                .backend_name = "wall-clock",
                .notes = "Measured with std::chrono::steady_clock.",
                .latency_measurement_kind =
                    fgep::telemetry::LatencyMeasurementKind::wall_clock_host,
                .warmup_count = 0
            }
        );

        assert(contains_text(report, "# Wall Clock Backend Benchmark"));
        assert(contains_text(report, "Backend: `wall-clock`"));
        assert(contains_text(report, "Measured with std::chrono::steady_clock."));
        assert(contains_text(report, "## Measurement notes"));
        assert(contains_text(report, "| latency_type | wall-clock host latency |"));
        assert(contains_text(report, "host clock-read overhead"));
        assert(contains_text(report, "## Wall-clock backend comparison"));
        assert(contains_text(report, "| recording | 100 | 100 | 0 | 50000 ns | 2000000 candidates/s | 200 | 500 | 500 |"));
        assert(contains_text(report, "| kernel_udp | 100 | 100 | 0 | 100000 ns | 1000000 candidates/s | 400 | 900 | 900 |"));
    }

    {
        WallClockBackendBenchmarkResult result{};

        const auto report = format_wall_clock_backend_benchmark_markdown(
            result,
            BenchmarkReportMetadata{
                .title = "Empty Wall Clock Backend Benchmark",
                .backend_name = "none",
                .notes = {},
                .latency_measurement_kind =
                    fgep::telemetry::LatencyMeasurementKind::wall_clock_host,
                .warmup_count = 0
            }
        );

        assert(contains_text(report, "# Empty Wall Clock Backend Benchmark"));
        assert(contains_text(report, "| latency_type | wall-clock host latency |"));
        assert(contains_text(report, "| n/a | 0 | 0 | 0 | 0 ns | 0 candidates/s | n/a | n/a | n/a |"));
    }

    return 0;
}