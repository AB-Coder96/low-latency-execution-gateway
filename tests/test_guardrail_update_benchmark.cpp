#include "fgep/gate/guardrail_update_benchmark.hpp"

#include <cassert>

int main() {
    using namespace fgep::gate;

    {
        const auto result = run_guardrail_update_benchmark(
            GuardrailUpdateBenchmarkConfig{
                .update_count = 0,
                .base_update_latency_ns = 1'000,
                .latency_jitter_step_ns = 25,
                .latency_jitter_period = 7
            }
        );

        assert(result.update_count == 0);
        assert(result.applied_count == 0);
        assert(result.dropped_count == 0);
        assert(result.simulated_elapsed_ns == 0);
        assert(!result.update_latency.has_value());
    }

    {
        const auto result = run_guardrail_update_benchmark(
            GuardrailUpdateBenchmarkConfig{
                .update_count = 4,
                .base_update_latency_ns = 1'000,
                .latency_jitter_step_ns = 25,
                .latency_jitter_period = 7
            }
        );

        assert(result.update_count == 4);
        assert(result.applied_count == 4);
        assert(result.dropped_count == 0);
        assert(result.simulated_elapsed_ns == 4'150);

        assert(result.update_latency.has_value());
        assert(result.update_latency->count == 4);
        assert(result.update_latency->min_ns == 1'000);
        assert(result.update_latency->max_ns == 1'075);
        assert(result.update_latency->mean_ns == 1'037);
        assert(result.update_latency->p50_ns == 1'025);
        assert(result.update_latency->p90_ns == 1'075);
        assert(result.update_latency->p99_ns == 1'075);
    }

    {
        const auto result = run_guardrail_update_benchmark(
            GuardrailUpdateBenchmarkConfig{
                .update_count = 3,
                .base_update_latency_ns = 500,
                .latency_jitter_step_ns = 100,
                .latency_jitter_period = 0
            }
        );

        assert(result.update_count == 3);
        assert(result.applied_count == 3);
        assert(result.dropped_count == 0);
        assert(result.simulated_elapsed_ns == 1'500);

        assert(result.update_latency.has_value());
        assert(result.update_latency->count == 3);
        assert(result.update_latency->min_ns == 500);
        assert(result.update_latency->max_ns == 500);
        assert(result.update_latency->mean_ns == 500);
        assert(result.update_latency->p99_ns == 500);
    }

    {
        const auto result = run_guardrail_update_benchmark(
            GuardrailUpdateBenchmarkConfig{}
        );

        assert(result.update_count == 1'000);
        assert(result.applied_count == 1'000);
        assert(result.dropped_count == 0);
        assert(result.update_latency.has_value());
        assert(result.update_latency->count == 1'000);
    }

    return 0;
}