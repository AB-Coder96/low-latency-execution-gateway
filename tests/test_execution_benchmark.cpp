#include "fgep/bench/execution_benchmark.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cassert>

namespace {

[[nodiscard]] fgep::ouch::Symbol symbol(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<8>(text);
    assert(value.ok());
    return value.value;
}

} // namespace

int main() {
    using namespace fgep::bench;

    {
        const auto result = run_execution_benchmark(ExecutionBenchmarkConfig{
            .candidate_count = 0,
            .generator_config = {},
            .risk_limits = {},
            .stage_durations = {}
        });

        assert(result.candidate_count == 0);
        assert(result.accepted_count == 0);
        assert(result.rejected_count == 0);
        assert(result.live_order_count == 0);
        assert(result.simulated_elapsed_ns == 0);
        assert(result.candidates_per_second == 0);
        assert(!result.end_to_end_latency.has_value());
    }

    {
        const auto result = run_execution_benchmark(ExecutionBenchmarkConfig{
            .candidate_count = 5,
            .generator_config = OrderCandidateGeneratorConfig{
                .symbols = {symbol("AAPL")},
                .first_user_ref_num = 1,
                .min_quantity = 100,
                .max_quantity = 100,
                .base_price = 100'000,
                .price_step = 0,
                .hot_symbol_count = 1,
                .hot_symbol_percent = 100,
                .alternate_sides = true
            },
            .risk_limits = fgep::risk::RiskLimits{
                .max_order_quantity = 1'000,
                .max_order_notional = 100'000'000ULL
            },
            .stage_durations = BenchmarkStageDurations{
                .ingest_to_decode_ns = 10,
                .decode_to_book_update_ns = 20,
                .book_update_to_risk_ns = 30,
                .risk_to_lifecycle_ns = 40,
                .lifecycle_to_enqueue_ns = 50,
                .enqueue_to_submit_ns = 60,
                .candidate_spacing_ns = 1'000
            }
        });

        assert(result.candidate_count == 5);
        assert(result.accepted_count == 5);
        assert(result.rejected_count == 0);
        assert(result.live_order_count == 5);
        assert(result.simulated_elapsed_ns == 4'210);
        assert(result.candidates_per_second == 1'187'648);

        assert(result.end_to_end_latency.has_value());
        assert(result.end_to_end_latency->count == 5);
        assert(result.end_to_end_latency->min_ns == 210);
        assert(result.end_to_end_latency->max_ns == 210);
        assert(result.end_to_end_latency->mean_ns == 210);
        assert(result.end_to_end_latency->p50_ns == 210);
        assert(result.end_to_end_latency->p99_ns == 210);
    }

    {
        const auto result = run_execution_benchmark(ExecutionBenchmarkConfig{
            .candidate_count = 4,
            .generator_config = OrderCandidateGeneratorConfig{
                .symbols = {symbol("AAPL")},
                .first_user_ref_num = 1,
                .min_quantity = 2'000,
                .max_quantity = 2'000,
                .base_price = 100'000,
                .price_step = 0,
                .hot_symbol_count = 1,
                .hot_symbol_percent = 100,
                .alternate_sides = true
            },
            .risk_limits = fgep::risk::RiskLimits{
                .max_order_quantity = 1'000,
                .max_order_notional = 100'000'000ULL
            },
            .stage_durations = {}
        });

        assert(result.candidate_count == 4);
        assert(result.accepted_count == 0);
        assert(result.rejected_count == 4);
        assert(result.live_order_count == 0);
        assert(result.end_to_end_latency.has_value());
        assert(result.end_to_end_latency->count == 4);
    }

    {
        const auto result = run_execution_benchmark(ExecutionBenchmarkConfig{
            .candidate_count = 3,
            .generator_config = OrderCandidateGeneratorConfig{
                .symbols = {symbol("AAPL")},
                .first_user_ref_num = 1,
                .min_quantity = 100,
                .max_quantity = 100,
                .base_price = 1'000'000,
                .price_step = 0,
                .hot_symbol_count = 1,
                .hot_symbol_percent = 100,
                .alternate_sides = true
            },
            .risk_limits = fgep::risk::RiskLimits{
                .max_order_quantity = 1'000,
                .max_order_notional = 50'000'000ULL
            },
            .stage_durations = {}
        });

        assert(result.candidate_count == 3);
        assert(result.accepted_count == 0);
        assert(result.rejected_count == 3);
        assert(result.live_order_count == 0);
    }

    return 0;
}