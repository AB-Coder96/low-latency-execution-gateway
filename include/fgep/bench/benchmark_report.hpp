#pragma once

#include "fgep/bench/execution_benchmark.hpp"

#include <string>
#include <string_view>

namespace fgep::bench {

struct BenchmarkReportMetadata {
    std::string title{"Execution Benchmark"};
    std::string backend_name{"simulated"};
    std::string notes{};
};

[[nodiscard]] std::string format_duration_ns(
    telemetry::DurationNs duration_ns
);

[[nodiscard]] std::string format_rate_per_second(
    std::uint64_t rate
);

[[nodiscard]] std::string format_execution_benchmark_markdown(
    const ExecutionBenchmarkResult& result,
    const BenchmarkReportMetadata& metadata = {}
);

[[nodiscard]] bool contains_text(
    std::string_view text,
    std::string_view needle
) noexcept;

} // namespace fgep::bench