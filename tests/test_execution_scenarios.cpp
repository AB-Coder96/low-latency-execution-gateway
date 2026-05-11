#include "fgep/execution/simulated_execution_engine.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

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

[[nodiscard]] fgep::execution::SimulatedExecutionEngine make_engine() {
    return fgep::execution::SimulatedExecutionEngine{
        fgep::risk::RiskSupervisor{fgep::risk::RiskLimits{
            .max_order_quantity = 1'000,
            .max_order_notional = 50'000'000ULL
        }}
    };
}

} // namespace

int main() {
    using namespace fgep::execution;

    std::size_t scenarios_passed = 0;

    // 1. Normal accepted enter.
    {
        auto engine = make_engine();

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

        ++scenarios_passed;
    }

    // 2. Duplicate live UserRefNum is rejected by lifecycle.
    {
        auto engine = make_engine();

        const auto first = engine.submit_enter(
            enter(1, "AAPL", 100, 100'000)
        );
        const auto duplicate = engine.submit_enter(
            enter(1, "AAPL", 100, 100'000)
        );

        assert(first.accepted());
        assert(duplicate.rejected());
        assert(duplicate.reject_source == ExecutionRejectSource::lifecycle);
        assert(
            duplicate.lifecycle_reject_reason
            == LifecycleRejectReason::duplicate_user_ref_num
        );
        assert(engine.live_order_count() == 1);

        ++scenarios_passed;
    }

    // 3. Stale UserRefNum is rejected after a higher value has been accepted.
    {
        auto engine = make_engine();

        const auto accepted = engine.submit_enter(
            enter(10, "AAPL", 100, 100'000)
        );
        const auto stale = engine.submit_enter(
            enter(9, "AAPL", 100, 100'000)
        );

        assert(accepted.accepted());
        assert(stale.rejected());
        assert(stale.reject_source == ExecutionRejectSource::lifecycle);
        assert(
            stale.lifecycle_reject_reason
            == LifecycleRejectReason::stale_user_ref_num
        );
        assert(!engine.contains(9));

        ++scenarios_passed;
    }

    // 4. Max quantity is rejected by risk before lifecycle.
    {
        auto engine = make_engine();

        const auto result = engine.submit_enter(
            enter(1, "AAPL", 1'001, 100'000)
        );

        assert(result.rejected());
        assert(result.reject_source == ExecutionRejectSource::risk);
        assert(
            result.risk_reject_reason
            == fgep::risk::RiskRejectReason::max_order_quantity
        );
        assert(engine.live_order_count() == 0);

        ++scenarios_passed;
    }

    // 5. Max notional is rejected by risk before lifecycle.
    {
        auto engine = make_engine();

        const auto result = engine.submit_enter(
            enter(1, "AAPL", 600, 100'000)
        );

        assert(result.rejected());
        assert(result.reject_source == ExecutionRejectSource::risk);
        assert(
            result.risk_reject_reason
            == fgep::risk::RiskRejectReason::max_order_notional
        );
        assert(engine.live_order_count() == 0);

        ++scenarios_passed;
    }

    // 6. Disabled symbol is rejected by risk.
    {
        auto engine = make_engine();

        engine.risk().disable_symbol(symbol("MSFT"));

        const auto result = engine.submit_enter(
            enter(1, "MSFT", 100, 100'000)
        );

        assert(result.rejected());
        assert(result.reject_source == ExecutionRejectSource::risk);
        assert(
            result.risk_reject_reason
            == fgep::risk::RiskRejectReason::symbol_disabled
        );
        assert(!engine.contains(1));

        ++scenarios_passed;
    }

    // 7. Cancel-only mode rejects new orders.
    {
        auto engine = make_engine();

        engine.risk().set_cancel_only(true);

        const auto result = engine.submit_enter(
            enter(1, "AAPL", 100, 100'000)
        );

        assert(result.rejected());
        assert(result.reject_source == ExecutionRejectSource::risk);
        assert(
            result.risk_reject_reason
            == fgep::risk::RiskRejectReason::cancel_only
        );
        assert(engine.live_order_count() == 0);

        ++scenarios_passed;
    }

    // 8. Cancel-only mode still allows cancel requests.
    {
        auto engine = make_engine();

        const auto accepted = engine.submit_enter(
            enter(1, "AAPL", 100, 100'000)
        );

        engine.risk().set_cancel_only(true);

        const auto canceled = engine.submit_cancel(cancel(1, 0));

        assert(accepted.accepted());
        assert(canceled.accepted());
        assert(canceled.action == ExecutionAction::cancel);
        assert(canceled.lifecycle_status == LifecycleStatus::canceled);
        assert(!engine.contains(1));

        ++scenarios_passed;
    }

    // 9. Replace removes the old UserRefNum and installs the new one.
    {
        auto engine = make_engine();

        const auto accepted = engine.submit_enter(
            enter(1, "AAPL", 100, 100'000)
        );
        const auto replaced = engine.submit_replace(
            replace(1, 2, 150, 110'000)
        );

        assert(accepted.accepted());
        assert(replaced.accepted());
        assert(replaced.action == ExecutionAction::replace);
        assert(replaced.lifecycle_status == LifecycleStatus::replaced);
        assert(replaced.previous_user_ref_num == 1);
        assert(replaced.user_ref_num == 2);
        assert(!engine.contains(1));
        assert(engine.contains(2));
        assert(engine.find(2)->leaves_quantity == 150);

        ++scenarios_passed;
    }

    // 10. Unknown replace is rejected by lifecycle.
    {
        auto engine = make_engine();

        const auto result = engine.submit_replace(
            replace(99, 100, 100, 100'000)
        );

        assert(result.rejected());
        assert(result.reject_source == ExecutionRejectSource::lifecycle);
        assert(
            result.lifecycle_reject_reason
            == LifecycleRejectReason::unknown_order
        );
        assert(!engine.contains(100));

        ++scenarios_passed;
    }

    // 11. Partial fill decrements leaves quantity.
    {
        auto engine = make_engine();

        const auto accepted = engine.submit_enter(
            enter(1, "AAPL", 100, 100'000)
        );
        const auto partial = engine.execute(1, 40);

        assert(accepted.accepted());
        assert(partial.accepted());
        assert(partial.action == ExecutionAction::execute);
        assert(partial.lifecycle_status == LifecycleStatus::partially_filled);
        assert(partial.decrement_quantity == 40);
        assert(partial.leaves_quantity == 60);
        assert(engine.contains(1));
        assert(engine.find(1)->leaves_quantity == 60);

        ++scenarios_passed;
    }

    // 12. Overfill is rejected and does not mutate leaves quantity.
    {
        auto engine = make_engine();

        const auto accepted = engine.submit_enter(
            enter(1, "AAPL", 100, 100'000)
        );
        const auto overfill = engine.execute(1, 101);

        assert(accepted.accepted());
        assert(overfill.rejected());
        assert(overfill.reject_source == ExecutionRejectSource::lifecycle);
        assert(overfill.lifecycle_reject_reason == LifecycleRejectReason::overfill);
        assert(engine.contains(1));
        assert(engine.find(1)->leaves_quantity == 100);

        ++scenarios_passed;
    }

    // 13. Full fill removes the order.
    {
        auto engine = make_engine();

        const auto accepted = engine.submit_enter(
            enter(1, "AAPL", 100, 100'000)
        );
        const auto filled = engine.execute(1, 100);

        assert(accepted.accepted());
        assert(filled.accepted());
        assert(filled.lifecycle_status == LifecycleStatus::filled);
        assert(filled.leaves_quantity == 0);
        assert(!engine.contains(1));
        assert(engine.live_order_count() == 0);

        ++scenarios_passed;
    }

    // 14. Global halt rejects new orders.
    {
        auto engine = make_engine();

        engine.risk().set_global_halted(true);

        const auto result = engine.submit_enter(
            enter(1, "AAPL", 100, 100'000)
        );

        assert(result.rejected());
        assert(result.reject_source == ExecutionRejectSource::risk);
        assert(
            result.risk_reject_reason
            == fgep::risk::RiskRejectReason::global_halt
        );
        assert(!engine.contains(1));

        ++scenarios_passed;
    }

    // 15. Global halt rejects cancel requests too.
    {
        auto engine = make_engine();

        const auto accepted = engine.submit_enter(
            enter(1, "AAPL", 100, 100'000)
        );

        engine.risk().set_global_halted(true);

        const auto result = engine.submit_cancel(cancel(1, 0));

        assert(accepted.accepted());
        assert(result.rejected());
        assert(result.reject_source == ExecutionRejectSource::risk);
        assert(
            result.risk_reject_reason
            == fgep::risk::RiskRejectReason::global_halt
        );
        assert(engine.contains(1));
        assert(engine.find(1)->leaves_quantity == 100);

        ++scenarios_passed;
    }

    assert(scenarios_passed == 15);

    return 0;
}