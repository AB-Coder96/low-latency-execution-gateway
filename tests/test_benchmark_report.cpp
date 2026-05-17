#include "fgep/bench/benchmark_report.hpp"

#include <cassert>

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
                .p999_ns = 210
            }
        };

        const auto report = format_execution_benchmark_markdown(
            result,
            BenchmarkReportMetadata{
                .title = "Deterministic Execution Benchmark",
                .backend_name = "simulated",
                .notes = "Synthetic deterministic timestamps."
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
                    .p999_ns = 1'100
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
                    .p999_ns = 1'100
                }
            }
        }
    };

    const auto report = format_backend_benchmark_markdown(
        result,
        BenchmarkReportMetadata{
            .title = "Backend Comparison",
            .backend_name = "multi-backend",
            .notes = "Deterministic backend comparison."
        }
    );

    assert(contains_text(report, "# Backend Comparison"));
    assert(contains_text(report, "Backend: `multi-backend`"));
    assert(contains_text(report, "Deterministic backend comparison."));
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
            .notes = {}
        }
    );

    assert(contains_text(report, "# Empty Backend Comparison"));
    assert(contains_text(report, "| n/a | 0 | 0 | 0 | 0 ns | 0 candidates/s | n/a | n/a | n/a |"));
}
    return 0;
}