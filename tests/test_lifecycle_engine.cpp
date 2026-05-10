#include "fgep/execution/order_lifecycle.hpp"
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

    OrderLifecycleEngine engine{};

    {
        const auto result = engine.enter(enter(1, 100, 1'902'500));

        assert(result.status == LifecycleStatus::accepted);
        assert(result.accepted());
        assert(result.reject_reason == LifecycleRejectReason::none);
        assert(result.user_ref_num == 1);
        assert(result.leaves_quantity == 100);
        assert(engine.live_order_count() == 1);
        assert(engine.highest_user_ref_num() == 1);
        assert(engine.contains(1));
    }

    {
        const auto result = engine.enter(enter(1, 100, 1'902'500));

        assert(result.status == LifecycleStatus::rejected);
        assert(result.rejected());
        assert(
            result.reject_reason
            == LifecycleRejectReason::duplicate_user_ref_num
        );
        assert(engine.live_order_count() == 1);
    }

    {
        const auto result = engine.replace(replace(1, 2, 150, 1'903'000));

        assert(result.status == LifecycleStatus::replaced);
        assert(result.previous_user_ref_num == 1);
        assert(result.user_ref_num == 2);
        assert(result.decrement_quantity == 100);
        assert(result.leaves_quantity == 150);
        assert(!engine.contains(1));
        assert(engine.contains(2));
        assert(engine.live_order_count() == 1);
        assert(engine.highest_user_ref_num() == 2);

        const auto* order = engine.find(2);
        assert(order != nullptr);
        assert(order->price == 1'903'000);
        assert(order->original_quantity == 150);
        assert(order->leaves_quantity == 150);
    }

    {
        const auto result = engine.replace(replace(99, 3, 100, 1'904'000));

        assert(result.status == LifecycleStatus::rejected);
        assert(result.reject_reason == LifecycleRejectReason::unknown_order);
        assert(!engine.contains(3));
    }

    {
        const auto result = engine.cancel(cancel(2, 70));

        assert(result.status == LifecycleStatus::partially_canceled);
        assert(result.decrement_quantity == 70);
        assert(result.leaves_quantity == 80);
        assert(engine.contains(2));
        assert(engine.find(2)->leaves_quantity == 80);
    }

    {
        const auto result = engine.execute(2, 30);

        assert(result.status == LifecycleStatus::partially_filled);
        assert(result.decrement_quantity == 30);
        assert(result.leaves_quantity == 50);
        assert(engine.contains(2));
        assert(engine.find(2)->leaves_quantity == 50);
    }

    {
        const auto result = engine.execute(2, 60);

        assert(result.status == LifecycleStatus::rejected);
        assert(result.reject_reason == LifecycleRejectReason::overfill);
        assert(engine.contains(2));
        assert(engine.find(2)->leaves_quantity == 50);
    }

    {
        const auto result = engine.cancel(cancel(2, 0));

        assert(result.status == LifecycleStatus::canceled);
        assert(result.decrement_quantity == 50);
        assert(result.leaves_quantity == 0);
        assert(!engine.contains(2));
        assert(engine.live_order_count() == 0);
    }

    {
        const auto result = engine.enter(enter(2, 100, 1'902'500));

        assert(result.status == LifecycleStatus::rejected);
        assert(result.reject_reason == LifecycleRejectReason::stale_user_ref_num);
    }

    {
        const auto accepted = engine.enter(enter(4, 40, 1'902'500));
        assert(accepted.status == LifecycleStatus::accepted);

        const auto filled = engine.execute(4, 40);
        assert(filled.status == LifecycleStatus::filled);
        assert(filled.leaves_quantity == 0);
        assert(!engine.contains(4));
    }

    {
        engine.clear();

        assert(engine.live_order_count() == 0);
        assert(engine.highest_user_ref_num() == 0);

        const auto accepted = engine.enter(enter(1, 10, 1'902'500));
        assert(accepted.status == LifecycleStatus::accepted);
        assert(engine.highest_user_ref_num() == 1);
    }

    return 0;
}