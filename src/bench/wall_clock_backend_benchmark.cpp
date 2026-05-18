#include "fgep/bench/wall_clock_backend_benchmark.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <limits>
#include <vector>

namespace fgep::bench {
namespace {

using Clock = std::chrono::steady_clock;

[[nodiscard]] std::vector<std::byte> make_payload(
    std::size_t payload_size
) {
    std::vector<std::byte> payload(payload_size);

    for (std::size_t index = 0; index < payload.size(); ++index) {
        payload[index] = static_cast<std::byte>(index & 0xffU);
    }

    return payload;
}

[[nodiscard]] telemetry::DurationNs to_duration_ns(
    Clock::duration duration
) noexcept {
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        duration
    ).count();

    if (ns <= 0) {
        return 0;
    }

    return static_cast<telemetry::DurationNs>(ns);
}

[[nodiscard]] std::uint64_t calculate_rate(
    std::size_t count,
    telemetry::DurationNs elapsed_ns
) noexcept {
    if (count == 0 || elapsed_ns == 0) {
        return 0;
    }

    const auto safe_count = static_cast<std::uint64_t>(count);

    if (
        safe_count
        > std::numeric_limits<std::uint64_t>::max() / 1'000'000'000ULL
    ) {
        return std::numeric_limits<std::uint64_t>::max();
    }

    return (safe_count * 1'000'000'000ULL) / elapsed_ns;
}

[[nodiscard]] execution::BackendFactoryConfig factory_config_for(
    const WallClockBackendBenchmarkConfig& config,
    execution::BackendTechnology technology
) {
    return execution::BackendFactoryConfig{
        .technology = technology,
        .kernel_udp = config.kernel_udp,
        .afxdp = config.afxdp,
        .dpdk = config.dpdk,
        .recording_max_submissions = 0
    };
}

} // namespace

WallClockBackendBenchmarkResult run_wall_clock_backend_benchmark(
    const WallClockBackendBenchmarkConfig& config
) {
    WallClockBackendBenchmarkResult result{};

    result.technologies.reserve(config.technologies.size());

    for (const auto technology : config.technologies) {
        result.technologies.push_back(
            run_wall_clock_backend_benchmark_for(config, technology)
        );
    }

    return result;
}

WallClockBackendTechnologyResult run_wall_clock_backend_benchmark_for(
    const WallClockBackendBenchmarkConfig& config,
    execution::BackendTechnology technology
) {
    auto backend = execution::make_execution_backend(
        factory_config_for(config, technology)
    );

    const auto payload = make_payload(config.payload_size);
    telemetry::LatencyRecorder recorder{};

    WallClockBackendTechnologyResult result{
        .technology = technology,
        .submission_count = config.submission_count,
        .accepted_count = 0,
        .rejected_count = 0,
        .elapsed_ns = 0,
        .submissions_per_second = 0,
        .submit_latency = std::nullopt
    };

    const auto batch_start = Clock::now();

    for (std::size_t index = 0; index < config.submission_count; ++index) {
        const auto request = execution::BackendSubmitRequest{
            .user_ref_num = static_cast<ouch::UserRefNum>(index + 1U),
            .kind = execution::BackendOrderKind::enter,
            .payload = payload
        };

        if (config.record_per_submit_latency) {
            const auto submit_start = Clock::now();
            const auto submit_result = backend->submit(request);
            const auto submit_end = Clock::now();

            recorder.record(to_duration_ns(submit_end - submit_start));

            if (submit_result.accepted()) {
                ++result.accepted_count;
            } else {
                ++result.rejected_count;
            }
        } else {
            const auto submit_result = backend->submit(request);

            if (submit_result.accepted()) {
                ++result.accepted_count;
            } else {
                ++result.rejected_count;
            }
        }
    }

    const auto batch_end = Clock::now();

    result.elapsed_ns = to_duration_ns(batch_end - batch_start);
    result.submissions_per_second = calculate_rate(
        result.submission_count,
        result.elapsed_ns
    );

    if (config.record_per_submit_latency) {
        result.submit_latency = recorder.summary();
    }

    return result;
}

} // namespace fgep::bench