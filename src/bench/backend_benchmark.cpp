#include "fgep/bench/backend_benchmark.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

namespace fgep::bench {
namespace {

[[nodiscard]] std::vector<std::byte> make_payload(
    std::size_t payload_size
) {
    std::vector<std::byte> payload(payload_size);

    for (std::size_t index = 0; index < payload.size(); ++index) {
        payload[index] = static_cast<std::byte>(index & 0xffU);
    }

    return payload;
}

[[nodiscard]] telemetry::DurationNs simulated_submit_latency(
    std::size_t index,
    const BackendBenchmarkConfig& config
) noexcept {
    if (config.latency_jitter_period == 0) {
        return config.base_submit_latency_ns;
    }

    const auto jitter_slot = static_cast<telemetry::DurationNs>(
        index % config.latency_jitter_period
    );

    return config.base_submit_latency_ns
        + (jitter_slot * config.latency_jitter_step_ns);
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
    const BackendBenchmarkConfig& config,
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

BackendBenchmarkResult run_backend_benchmark(
    const BackendBenchmarkConfig& config
) {
    BackendBenchmarkResult result{};

    result.technologies.reserve(config.technologies.size());

    for (const auto technology : config.technologies) {
        result.technologies.push_back(
            run_backend_benchmark_for(config, technology)
        );
    }

    return result;
}

BackendBenchmarkTechnologyResult run_backend_benchmark_for(
    const BackendBenchmarkConfig& config,
    execution::BackendTechnology technology
) {
    auto backend = execution::make_execution_backend(
        factory_config_for(config, technology)
    );

    const auto payload = make_payload(config.payload_size);
    telemetry::LatencyRecorder recorder{};

    BackendBenchmarkTechnologyResult result{
        .technology = technology,
        .submission_count = config.submission_count,
        .accepted_count = 0,
        .rejected_count = 0,
        .simulated_elapsed_ns = 0,
        .submissions_per_second = 0,
        .submit_latency = std::nullopt
    };

    for (std::size_t index = 0; index < config.submission_count; ++index) {
        const auto submit_result = backend->submit(
            execution::BackendSubmitRequest{
                .user_ref_num = static_cast<ouch::UserRefNum>(index + 1U),
                .kind = execution::BackendOrderKind::enter,
                .payload = payload
            }
        );

        if (submit_result.accepted()) {
            ++result.accepted_count;
        } else {
            ++result.rejected_count;
        }

        const auto latency = simulated_submit_latency(index, config);
        result.simulated_elapsed_ns += latency;
        recorder.record(latency);
    }

    result.submissions_per_second = calculate_rate(
        result.submission_count,
        result.simulated_elapsed_ns
    );
    result.submit_latency = recorder.summary();

    return result;
}

} // namespace fgep::bench