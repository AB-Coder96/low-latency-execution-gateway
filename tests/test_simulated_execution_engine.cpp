#include "fgep/execution/simulated_execution_engine.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cassert>

namespace {

[[nodiscard]] fgep::ouch::Symbol symbol(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<8>(text);
    assert(value.ok());
    return value.value;
}

[[nodiscard]] fgep::ouch::ClOrdId cl_ord_id(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<14>(text);
    assert(value.ok());
    return value.value;
}

[[nodiscard]] fgep::ouch::EnterOrderMessage enter(
    fgep::ouch::UserRefNum user_ref_num,
    const char* stock,
    fgep::ouch::Quantity quantity,
    fgep::ouch::Price4 price
) {
    return fgep::ouch::EnterOrderMessage{
        .user_ref_num = user_ref_num,
        .side = fgep::ouch::Side::buy,
        .quantity = quantity,
        .symbol = symbol(stock),
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

[[nodiscard]] fgep::ouch::ReplaceOrderMessage replace(
    fgep::ouch::UserRefNum original_user_ref_num,
    fgep::ouch::UserRefNum replacement_user_ref_num,
    fgep::ouch::Quantity quantity,
    fgep::ouch::Price4 price
) {
    return fgep::ouch::ReplaceOrderMessage{
        .orig_user_ref_num = original_user_ref_num,
        .user_ref_num = replacement_user_ref_num,
        .quantity = quantity,
        .price = price,
        .time_in_force = fgep::ouch::TimeInForce::day,
        .display = fgep::ouch::Display::visible,
        .intermarket_sweep_eligibility =
            fgep::ouch::IntermarketSweepEligibility::not_eligible,
        .cl_ord_id = cl_ord_id("REPLACE1"),
        .optional_appendage = {}
    };
}

[[nodiscard]] fgep::ouch::CancelOrderMessage cancel(
    fgep::ouch::UserRefNum user_ref_num,
    fgep::ouch::Quantity quantity
) {
    return fgep::ouch::CancelOrderMessage{
        .user_ref_num = user_ref_num,
        .quantity = quantity,
        .optional_appendage = {}
    };
}

} // namespace

int main() {
    using namespace fgep::execution;

    SimulatedExecutionEngine engine{
        fgep::risk::RiskSupervisor{fgep::risk::RiskLimits{
            .max_order_quantity = 1'000,
            .max_order_notional = 50'000'000ULL
        }}
    };

    {
        const auto result = engine.submit_enter(
            enter(1, "AAPL", 100, 100'000)
        );

        assert(result.accepted());
        assert(result.action == ExecutionAction::enter);
        assert(result.reject_source == ExecutionRejectSource::none);
        assert(result.lifecycle_status == LifecycleStatus::accepted);
        assert(result.user_ref_num == 1);
        assert(result.leaves_quantity == 100);
        assert(engine.contains(1));
        assert(engine.live_order_count() == 1);
    }

    {
        const auto result = engine.submit_enter(
            enter(2, "AAPL", 2'000, 100'000)
        );

        assert(result.rejected());
        assert(result.reject_source == ExecutionRejectSource::risk);
        assert(
            result.risk_reject_reason
            == fgep::risk::RiskRejectReason::max_order_quantity
        );
        assert(!engine.contains(2));
        assert(engine.live_order_count() == 1);
    }

    {
        const auto result = engine.submit_replace(
            replace(1, 3, 150, 110'000)
        );

        assert(result.accepted());
        assert(result.action == ExecutionAction::replace);
        assert(result.lifecycle_status == LifecycleStatus::replaced);
        assert(result.previous_user_ref_num == 1);
        assert(result.user_ref_num == 3);
        assert(!engine.contains(1));
        assert(engine.contains(3));
        assert(engine.find(3)->leaves_quantity == 150);
    }

    {
        const auto result = engine.submit_replace(
            replace(99, 4, 100, 100'000)
        );

        assert(result.rejected());
        assert(result.reject_source == ExecutionRejectSource::lifecycle);
        assert(
            result.lifecycle_reject_reason
            == LifecycleRejectReason::unknown_order
        );
        assert(!engine.contains(4));
    }

    {
        engine.risk().set_cancel_only(true);

        const auto enter_result = engine.submit_enter(
            enter(5, "AAPL", 100, 100'000)
        );

        assert(enter_result.rejected());
        assert(enter_result.reject_source == ExecutionRejectSource::risk);
        assert(
            enter_result.risk_reject_reason
            == fgep::risk::RiskRejectReason::cancel_only
        );
        assert(!engine.contains(5));

        const auto cancel_result = engine.submit_cancel(cancel(3, 50));

        assert(cancel_result.accepted());
        assert(cancel_result.action == ExecutionAction::cancel);
        assert(cancel_result.lifecycle_status == LifecycleStatus::partially_canceled);
        assert(cancel_result.leaves_quantity == 100);
        assert(engine.contains(3));
        assert(engine.find(3)->leaves_quantity == 100);

        engine.risk().set_cancel_only(false);
    }

    {
        const auto fill_result = engine.execute(3, 40);

        assert(fill_result.accepted());
        assert(fill_result.action == ExecutionAction::execute);
        assert(fill_result.lifecycle_status == LifecycleStatus::partially_filled);
        assert(fill_result.decrement_quantity == 40);
        assert(fill_result.leaves_quantity == 60);
        assert(engine.contains(3));
    }

    {
        engine.risk().set_global_halted(true);

        const auto cancel_result = engine.submit_cancel(cancel(3, 0));

        assert(cancel_result.rejected());
        assert(cancel_result.reject_source == ExecutionRejectSource::risk);
        assert(
            cancel_result.risk_reject_reason
            == fgep::risk::RiskRejectReason::global_halt
        );
        assert(engine.contains(3));
        assert(engine.find(3)->leaves_quantity == 60);

        engine.risk().set_global_halted(false);
    }

    {
        const auto fill_result = engine.execute(3, 60);

        assert(fill_result.accepted());
        assert(fill_result.lifecycle_status == LifecycleStatus::filled);
        assert(fill_result.leaves_quantity == 0);
        assert(!engine.contains(3));
        assert(engine.live_order_count() == 0);
    }

    {
        engine.risk().disable_symbol(symbol("MSFT"));

        const auto result = engine.submit_enter(
            enter(6, "MSFT", 100, 100'000)
        );

        assert(result.rejected());
        assert(result.reject_source == ExecutionRejectSource::risk);
        assert(
            result.risk_reject_reason
            == fgep::risk::RiskRejectReason::symbol_disabled
        );
        assert(!engine.contains(6));
    }

    return 0;
}