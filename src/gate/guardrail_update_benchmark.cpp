#include "fgep/gate/guardrail_update_benchmark.hpp"

namespace fgep::gate {
namespace {

[[nodiscard]] telemetry::DurationNs simulated_update_latency(
    std::size_t index,
    const GuardrailUpdateBenchmarkConfig& config
) noexcept {
    if (config.latency_jitter_period == 0) {
        return config.base_update_latency_ns;
    }

    const auto jitter_slot = static_cast<telemetry::DurationNs>(
        index % config.latency_jitter_period
    );

    return config.base_update_latency_ns
        + (jitter_slot * config.latency_jitter_step_ns);
}

[[nodiscard]] GuardrailUpdate make_update_for_index(
    GuardrailControlPath& control,
    std::size_t index
) {
    switch (index % 4U) {
    case 0:
        return control.push_set_global_halt(false);

    case 1:
        return control.push_set_cancel_only(false);

    case 2:
        return control.push_set_default_symbol_enabled(true);

    default:
        return control.push_set_limits(GuardrailLimits{
            .max_order_quantity = 100'000,
            .max_order_notional = 1'000'000'000'000ULL
        });
    }
}

} // namespace

GuardrailUpdateBenchmarkResult run_guardrail_update_benchmark(
    const GuardrailUpdateBenchmarkConfig& config
) {
    GuardrailState state{};
    GuardrailControlPath control{};
    telemetry::LatencyRecorder latency_recorder{};

    GuardrailUpdateBenchmarkResult result{
        .update_count = config.update_count,
        .applied_count = 0,
        .dropped_count = 0,
        .simulated_elapsed_ns = 0,
        .update_latency = std::nullopt
    };

    for (std::size_t index = 0; index < config.update_count; ++index) {
        static_cast<void>(make_update_for_index(control, index));

        const auto latency = simulated_update_latency(index, config);
        result.simulated_elapsed_ns += latency;

        const auto apply_result = control.apply_next(state);

        if (apply_result.applied()) {
            ++result.applied_count;
            latency_recorder.record(latency);
        } else if (apply_result.dropped()) {
            ++result.dropped_count;
        }
    }

    result.update_latency = latency_recorder.summary();

    return result;
}

} // namespace fgep::gate