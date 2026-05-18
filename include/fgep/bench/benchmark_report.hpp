#pragma once

#include "fgep/bench/execution_benchmark.hpp"
#include "fgep/bench/backend_benchmark.hpp"
#include "fgep/bench/wall_clock_backend_benchmark.hpp"
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

[[nodiscard]] std::string format_wall_clock_backend_benchmark_markdown(
    const WallClockBackendBenchmarkResult& result,
    const BenchmarkReportMetadata& metadata = {}
);


[[nodiscard]] bool contains_text(
    std::string_view text,
    std::string_view needle
) noexcept;

[[nodiscard]] std::string format_backend_benchmark_markdown(
    const BackendBenchmarkResult& result,
    const BenchmarkReportMetadata& metadata = {}
);
} // namespace fgep::bench