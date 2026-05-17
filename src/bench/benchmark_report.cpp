#include "fgep/bench/benchmark_report.hpp"

#include <sstream>
#include <string>

namespace fgep::bench {
namespace {

void append_latency_summary(
    std::ostringstream& output,
    const telemetry::LatencySummary& summary
) {
    output << "| count | " << summary.count << " |\n";
    output << "| min_ns | " << summary.min_ns << " |\n";
    output << "| max_ns | " << summary.max_ns << " |\n";
    output << "| mean_ns | " << summary.mean_ns << " |\n";
    output << "| p50_ns | " << summary.p50_ns << " |\n";
    output << "| p90_ns | " << summary.p90_ns << " |\n";
    output << "| p99_ns | " << summary.p99_ns << " |\n";
    output << "| p999_ns | " << summary.p999_ns << " |\n";
}

} // namespace

std::string format_duration_ns(telemetry::DurationNs duration_ns) {
    return std::to_string(duration_ns) + " ns";
}

std::string format_rate_per_second(std::uint64_t rate) {
    return std::to_string(rate) + " candidates/s";
}

std::string format_execution_benchmark_markdown(
    const ExecutionBenchmarkResult& result,
    const BenchmarkReportMetadata& metadata
) {
    std::ostringstream output{};

    output << "# " << metadata.title << "\n\n";
    output << "Backend: `" << metadata.backend_name << "`\n\n";

    if (!metadata.notes.empty()) {
        output << metadata.notes << "\n\n";
    }

    output << "## Summary\n\n";
    output << "| Metric | Value |\n";
    output << "|---|---:|\n";
    output << "| candidate_count | " << result.candidate_count << " |\n";
    output << "| accepted_count | " << result.accepted_count << " |\n";
    output << "| rejected_count | " << result.rejected_count << " |\n";
    output << "| live_order_count | " << result.live_order_count << " |\n";
    output << "| simulated_elapsed | "
           << format_duration_ns(result.simulated_elapsed_ns) << " |\n";
    output << "| throughput | "
           << format_rate_per_second(result.candidates_per_second) << " |\n";

    output << "\n## End-to-end latency\n\n";

    if (!result.end_to_end_latency.has_value()) {
        output << "No latency samples recorded.\n";
        return output.str();
    }

    output << "| Metric | Value |\n";
    output << "|---|---:|\n";
    append_latency_summary(output, result.end_to_end_latency.value());

    return output.str();
}

bool contains_text(
    std::string_view text,
    std::string_view needle
) noexcept {
    return text.find(needle) != std::string_view::npos;
}

} // namespace fgep::bench