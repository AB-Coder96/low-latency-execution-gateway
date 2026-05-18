#include "fgep/bench/wall_clock_backend_benchmark.hpp"

#include <cassert>

int main() {
    using namespace fgep::bench;
    using namespace fgep::execution;

    {
        const auto result = run_wall_clock_backend_benchmark_for(
            WallClockBackendBenchmarkConfig{
                .technologies = {},
                .submission_count = 0,
                .payload_size = 64,
                .record_per_submit_latency = true,
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
        assert(result.submissions_per_second == 0);
        assert(!result.submit_latency.has_value());
    }

    {
        const auto result = run_wall_clock_backend_benchmark_for(
            WallClockBackendBenchmarkConfig{
                .technologies = {},
                .submission_count = 100,
                .payload_size = 64,
                .record_per_submit_latency = true,
                .kernel_udp = {},
                .afxdp = {},
                .dpdk = {}
            },
            BackendTechnology::recording
        );

        assert(result.technology == BackendTechnology::recording);
        assert(result.submission_count == 100);
        assert(result.accepted_count == 100);
        assert(result.rejected_count == 0);
        assert(result.elapsed_ns > 0);
        assert(result.submissions_per_second > 0);
        assert(result.submit_latency.has_value());
        assert(result.submit_latency->count == 100);
        assert(result.submit_latency->max_ns >= result.submit_latency->min_ns);
    }

    {
        const auto result = run_wall_clock_backend_benchmark_for(
            WallClockBackendBenchmarkConfig{
                .technologies = {},
                .submission_count = 100,
                .payload_size = 0,
                .record_per_submit_latency = true,
                .kernel_udp = {},
                .afxdp = {},
                .dpdk = {}
            },
            BackendTechnology::recording
        );

        assert(result.technology == BackendTechnology::recording);
        assert(result.submission_count == 100);
        assert(result.accepted_count == 0);
        assert(result.rejected_count == 100);
        assert(result.elapsed_ns > 0);
        assert(result.submissions_per_second > 0);
        assert(result.submit_latency.has_value());
        assert(result.submit_latency->count == 100);
    }

    {
        const auto result = run_wall_clock_backend_benchmark_for(
            WallClockBackendBenchmarkConfig{
                .technologies = {},
                .submission_count = 100,
                .payload_size = 64,
                .record_per_submit_latency = false,
                .kernel_udp = {},
                .afxdp = {},
                .dpdk = {}
            },
            BackendTechnology::recording
        );

        assert(result.technology == BackendTechnology::recording);
        assert(result.submission_count == 100);
        assert(result.accepted_count == 100);
        assert(result.rejected_count == 0);
        assert(result.elapsed_ns > 0);
        assert(result.submissions_per_second > 0);
        assert(!result.submit_latency.has_value());
    }

    {
        const auto result = run_wall_clock_backend_benchmark(
            WallClockBackendBenchmarkConfig{
                .technologies = {
                    BackendTechnology::recording,
                    BackendTechnology::afxdp,
                    BackendTechnology::dpdk
                },
                .submission_count = 10,
                .payload_size = 64,
                .record_per_submit_latency = true,
                .kernel_udp = {},
                .afxdp = AfXdpBackendConfig{
                    .interface_name = "eth0",
                    .queue_id = 0,
                    .umem_frame_count = 4096,
                    .umem_frame_size = 2048,
                    .tx_batch_size = 64
                },
                .dpdk = {}
            }
        );

        assert(result.technologies.size() == 3);

        assert(result.technologies[0].technology == BackendTechnology::recording);
        assert(result.technologies[0].accepted_count == 10);
        assert(result.technologies[0].rejected_count == 0);

        assert(result.technologies[1].technology == BackendTechnology::afxdp);
        assert(result.technologies[1].accepted_count == 0);
        assert(result.technologies[1].rejected_count == 10);

        assert(result.technologies[2].technology == BackendTechnology::dpdk);
        assert(result.technologies[2].submission_count == 10);
        assert(result.technologies[2].submit_latency.has_value());
    }

    return 0;
}