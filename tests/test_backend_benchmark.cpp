#include "fgep/bench/backend_benchmark.hpp"

#include <cassert>

int main() {
    using namespace fgep::bench;
    using namespace fgep::execution;

    {
        const auto result = run_backend_benchmark(BackendBenchmarkConfig{
            .technologies = {
                BackendTechnology::recording,
                BackendTechnology::afxdp,
                BackendTechnology::dpdk
            },
            .submission_count = 3,
            .payload_size = 4,
            .base_submit_latency_ns = 1'000,
            .latency_jitter_step_ns = 100,
            .latency_jitter_period = 2,
            .kernel_udp = {},
            .afxdp = AfXdpBackendConfig{
                .interface_name = "eth0",
                .queue_id = 0,
                .umem_frame_count = 4096,
                .umem_frame_size = 2048,
                .tx_batch_size = 64
            },
            .dpdk = DpdkBackendConfig{}
        });

        assert(result.technologies.size() == 3);

        assert(result.technologies[0].technology == BackendTechnology::recording);
        assert(result.technologies[0].submission_count == 3);
        assert(result.technologies[0].accepted_count == 3);
        assert(result.technologies[0].rejected_count == 0);
        assert(result.technologies[0].simulated_elapsed_ns == 3'100);
        assert(result.technologies[0].submissions_per_second == 967'741);
        assert(result.technologies[0].submit_latency.has_value());
        assert(result.technologies[0].submit_latency->count == 3);
        assert(result.technologies[0].submit_latency->min_ns == 1'000);
        assert(result.technologies[0].submit_latency->max_ns == 1'100);

        assert(result.technologies[1].technology == BackendTechnology::afxdp);
        assert(result.technologies[1].submission_count == 3);
        assert(result.technologies[1].accepted_count == 0);
        assert(result.technologies[1].rejected_count == 3);
        assert(result.technologies[1].submit_latency.has_value());

        assert(result.technologies[2].technology == BackendTechnology::dpdk);
        assert(result.technologies[2].submission_count == 3);
        assert(result.technologies[2].submit_latency.has_value());
    }

    {
        const auto result = run_backend_benchmark_for(
            BackendBenchmarkConfig{
                .technologies = {},
                .submission_count = 0,
                .payload_size = 4,
                .base_submit_latency_ns = 1'000,
                .latency_jitter_step_ns = 100,
                .latency_jitter_period = 2,
                .kernel_udp = {},
                .afxdp = {},
                .dpdk = {}
            },
            BackendTechnology::recording
        );

        assert(result.technology == BackendTechnology::recording);
        assert(result.submission_count == 0);
        assert(result.accepted_count == 0);
        assert(result.rejected_count == 0);
        assert(result.simulated_elapsed_ns == 0);
        assert(result.submissions_per_second == 0);
        assert(!result.submit_latency.has_value());
    }

    {
        const auto result = run_backend_benchmark_for(
            BackendBenchmarkConfig{
                .technologies = {},
                .submission_count = 2,
                .payload_size = 0,
                .base_submit_latency_ns = 500,
                .latency_jitter_step_ns = 0,
                .latency_jitter_period = 0,
                .kernel_udp = {},
                .afxdp = {},
                .dpdk = {}
            },
            BackendTechnology::recording
        );

        assert(result.technology == BackendTechnology::recording);
        assert(result.submission_count == 2);
        assert(result.accepted_count == 0);
        assert(result.rejected_count == 2);
        assert(result.simulated_elapsed_ns == 1'000);
        assert(result.submissions_per_second == 2'000'000);
        assert(result.submit_latency.has_value());
        assert(result.submit_latency->count == 2);
        assert(result.submit_latency->mean_ns == 500);
    }

    return 0;
}