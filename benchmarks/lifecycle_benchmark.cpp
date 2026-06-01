#include "hotpath_benchmark.hpp"

#include "fgep/execution/order_lifecycle.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cstdint>

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
    fgep::ouch::UserRefNum user_ref_num
) {
    return fgep::ouch::EnterOrderMessage{
        .user_ref_num = user_ref_num,
        .side = fgep::ouch::Side::buy,
        .quantity = 100,
        .symbol = symbol("AAPL"),
        .price = 1'902'500,
        .time_in_force = fgep::ouch::TimeInForce::day,
        .display = fgep::ouch::Display::visible,
        .capacity = fgep::ouch::Capacity::agency,
        .intermarket_sweep_eligibility =
            fgep::ouch::IntermarketSweepEligibility::not_eligible,
        .cross_type = fgep::ouch::CrossType::continuous_market,
        .cl_ord_id = cl_ord_id("HOTPATH"),
        .optional_appendage = {}
    };
}

} // namespace

int main() {
    constexpr std::uint64_t iterations = 5'000'000U;

    fgep::execution::OrderLifecycleEngine engine{};
    fgep::ouch::UserRefNum next_ref = 1U;

    const auto result = fgep::benchmarks::run_hot_loop(
        "hotpath_lifecycle_enter",
        iterations,
        [&](std::uint64_t) {
            const auto message = enter_order(next_ref++);
            const auto lifecycle_result = engine.enter(message);

            fgep::benchmarks::do_not_optimize(lifecycle_result);
            fgep::benchmarks::clobber_memory();

            if (engine.live_order_count() >= 4096U) {
                engine.clear();
                next_ref = 1U;
            }
        }
    );

    fgep::benchmarks::print_result(result);
    return 0;
}