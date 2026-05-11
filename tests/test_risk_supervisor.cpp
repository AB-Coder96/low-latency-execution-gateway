#include "fgep/risk/risk_supervisor.hpp"
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
    using namespace fgep::risk;

    RiskSupervisor risk{RiskLimits{
        .max_order_quantity = 1'000,
        .max_order_notional = 50'000'000ULL
    }};

    {
        const auto result = risk.check_enter(enter(1, "AAPL", 100, 100'000));

        assert(result.accepted());
        assert(result.reject_reason == RiskRejectReason::none);
        assert(result.user_ref_num == 1);
        assert(result.notional == 10'000'000ULL);
    }

    {
        const auto result = risk.check_enter(enter(2, "AAPL", 1'001, 100'000));

        assert(result.rejected());
        assert(result.reject_reason == RiskRejectReason::max_order_quantity);
    }

    {
        const auto result = risk.check_enter(enter(3, "AAPL", 600, 100'000));

        assert(result.rejected());
        assert(result.reject_reason == RiskRejectReason::max_order_notional);
        assert(result.notional == 60'000'000ULL);
    }

    {
        risk.disable_symbol(symbol("MSFT"));

        const auto result = risk.check_enter(enter(4, "MSFT", 100, 100'000));

        assert(result.rejected());
        assert(result.reject_reason == RiskRejectReason::symbol_disabled);

        risk.enable_symbol(symbol("MSFT"));

        const auto accepted = risk.check_enter(enter(5, "MSFT", 100, 100'000));
        assert(accepted.accepted());
    }

    {
        risk.set_default_symbol_enabled(false);
        risk.enable_symbol(symbol("AAPL"));

        const auto accepted = risk.check_enter(enter(6, "AAPL", 100, 100'000));
        const auto rejected = risk.check_enter(enter(7, "TSLA", 100, 100'000));

        assert(accepted.accepted());
        assert(rejected.rejected());
        assert(rejected.reject_reason == RiskRejectReason::symbol_disabled);

        risk.set_default_symbol_enabled(true);
        risk.clear_symbol_overrides();
    }

    {
        risk.set_cancel_only(true);

        const auto enter_result = risk.check_enter(
            enter(8, "AAPL", 100, 100'000)
        );
        const auto replace_result = risk.check_replace(
            replace(1, 9, 100, 100'000),
            symbol("AAPL")
        );
        const auto cancel_result = risk.check_cancel(cancel(1, 0));

        assert(enter_result.rejected());
        assert(enter_result.reject_reason == RiskRejectReason::cancel_only);

        assert(replace_result.rejected());
        assert(replace_result.reject_reason == RiskRejectReason::cancel_only);

        assert(cancel_result.accepted());

        risk.set_cancel_only(false);
    }

    {
        risk.set_global_halted(true);

        const auto enter_result = risk.check_enter(
            enter(10, "AAPL", 100, 100'000)
        );
        const auto cancel_result = risk.check_cancel(cancel(1, 0));

        assert(enter_result.rejected());
        assert(enter_result.reject_reason == RiskRejectReason::global_halt);

        assert(cancel_result.rejected());
        assert(cancel_result.reject_reason == RiskRejectReason::global_halt);

        risk.set_global_halted(false);
    }

    {
        const auto result = risk.check_replace(
            replace(10, 11, 100, 100'000),
            symbol("AAPL")
        );

        assert(result.accepted());
        assert(result.notional == 10'000'000ULL);
    }

    {
        const auto result = risk.check_cancel(cancel(12, 1'000'000));

        assert(result.rejected());
        assert(result.reject_reason == RiskRejectReason::invalid_quantity);
    }

    return 0;
}
