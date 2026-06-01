#include "fgep/bench/order_candidate_generator.hpp"
#include "fgep/execution/order_lifecycle.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <benchmark/benchmark.h>

namespace {

[[nodiscard]] fgep::ouch::Symbol symbol(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<8>(text);

    if (value.failed()) {
        return {};
    }

    return value.value;
}

[[nodiscard]] fgep::ouch::ClOrdId cl_ord_id(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<14>(text);

    if (value.failed()) {
        return {};
    }

    return value.value;
}

[[nodiscard]] fgep::ouch::EnterOrderMessage enter_order(
    fgep::ouch::UserRefNum user_ref_num,
    fgep::ouch::Quantity quantity,
    fgep::ouch::Price4 price
) {
    return fgep::ouch::EnterOrderMessage{
        .user_ref_num = user_ref_num,
        .side = fgep::ouch::Side::buy,
        .quantity = quantity,
        .symbol = symbol("AAPL"),
        .price = price,
        .time_in_force = fgep::ouch::TimeInForce::day,
        .display = fgep::ouch::Display::visible,
        .capacity = fgep::ouch::Capacity::agency,
        .intermarket_sweep_eligibility =
            fgep::ouch::IntermarketSweepEligibility::not_eligible,
        .cross_type = fgep::ouch::CrossType::continuous_market,
        .cl_ord_id = cl_ord_id("ORDER1"),
        .optional_appendage = {}
    };
}

[[nodiscard]] fgep::ouch::ReplaceOrderMessage replace_order(
    fgep::ouch::UserRefNum original_user_ref_num,
    fgep::ouch::UserRefNum replacement_user_ref_num
) {
    return fgep::ouch::ReplaceOrderMessage{
        .orig_user_ref_num = original_user_ref_num,
        .user_ref_num = replacement_user_ref_num,
        .quantity = 150,
        .price = 1'903'000,
        .time_in_force = fgep::ouch::TimeInForce::day,
        .display = fgep::ouch::Display::visible,
        .intermarket_sweep_eligibility =
            fgep::ouch::IntermarketSweepEligibility::not_eligible,
        .cl_ord_id = cl_ord_id("REPLACE1"),
        .optional_appendage = {}
    };
}

[[nodiscard]] fgep::ouch::CancelOrderMessage cancel_order(
    fgep::ouch::UserRefNum user_ref_num
) {
    return fgep::ouch::CancelOrderMessage{
        .user_ref_num = user_ref_num,
        .quantity = 0,
        .optional_appendage = {}
    };
}

void BM_OrderCandidateGeneratorNextEnterOrder(benchmark::State& state) {
    fgep::bench::OrderCandidateGenerator generator{
        fgep::bench::OrderCandidateGeneratorConfig{
            .symbols = {
                symbol("AAPL"),
                symbol("MSFT"),
                symbol("TSLA"),
                symbol("NVDA")
            },
            .first_user_ref_num = 1,
            .min_quantity = 100,
            .max_quantity = 1'000,
            .base_price = 100'000,
            .price_step = 25,
            .hot_symbol_count = 2,
            .hot_symbol_percent = 80,
            .alternate_sides = true
        }
    };

    for (auto _ : state) {
        auto order = generator.next_enter_order();
        benchmark::DoNotOptimize(order);
    }
}

void BM_OrderLifecycleEnter(benchmark::State& state) {
    fgep::execution::OrderLifecycleEngine engine{};
    fgep::ouch::UserRefNum user_ref_num = 1;

    for (auto _ : state) {
        const auto message = enter_order(user_ref_num++, 100, 1'902'500);
        const auto result = engine.enter(message);

        benchmark::DoNotOptimize(result);

        if (engine.live_order_count() >= 4096U) {
            state.PauseTiming();
            engine.clear();
            user_ref_num = 1;
            state.ResumeTiming();
        }
    }
}

void BM_OrderLifecycleReplace(benchmark::State& state) {
    fgep::ouch::UserRefNum user_ref_num = 1;

    for (auto _ : state) {
        state.PauseTiming();

        fgep::execution::OrderLifecycleEngine engine{};
        const auto original_ref = user_ref_num++;
        const auto replacement_ref = user_ref_num++;

        benchmark::DoNotOptimize(
            engine.enter(enter_order(original_ref, 100, 1'902'500))
        );

        state.ResumeTiming();

        const auto result =
            engine.replace(replace_order(original_ref, replacement_ref));

        benchmark::DoNotOptimize(result);
    }
}

void BM_OrderLifecycleCancel(benchmark::State& state) {
    fgep::ouch::UserRefNum user_ref_num = 1;

    for (auto _ : state) {
        state.PauseTiming();

        fgep::execution::OrderLifecycleEngine engine{};
        const auto ref = user_ref_num++;

        benchmark::DoNotOptimize(
            engine.enter(enter_order(ref, 100, 1'902'500))
        );

        state.ResumeTiming();

        const auto result = engine.cancel(cancel_order(ref));
        benchmark::DoNotOptimize(result);
    }
}

} // namespace

BENCHMARK(BM_OrderCandidateGeneratorNextEnterOrder);
BENCHMARK(BM_OrderLifecycleEnter);
BENCHMARK(BM_OrderLifecycleReplace);
BENCHMARK(BM_OrderLifecycleCancel);