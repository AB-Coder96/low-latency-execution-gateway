#include "fgep/gate/guardrail_state.hpp"
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
    using namespace fgep::gate;

    GuardrailState guardrail{GuardrailLimits{
        .max_order_quantity = 1'000,
        .max_order_notional = 50'000'000ULL
    }};

    {
        const auto order = make_enter_guardrail_order(
            enter(1, "AAPL", 100, 100'000)
        );

        const auto result = guardrail.check(order);

        assert(result.allowed());
        assert(!result.rejected());
        assert(result.reject_reason == GuardrailRejectReason::none);
        assert(result.kind == GuardrailOrderKind::enter);
        assert(result.user_ref_num == 1);
        assert(result.notional == 10'000'000ULL);
    }

    {
        const auto order = make_enter_guardrail_order(
            enter(2, "AAPL", 1'001, 100'000)
        );

        const auto result = guardrail.check(order);

        assert(result.rejected());
        assert(result.reject_reason == GuardrailRejectReason::max_order_quantity);
    }

    {
        const auto order = make_enter_guardrail_order(
            enter(3, "AAPL", 600, 100'000)
        );

        const auto result = guardrail.check(order);

        assert(result.rejected());
        assert(result.reject_reason == GuardrailRejectReason::max_order_notional);
        assert(result.notional == 60'000'000ULL);
    }

    {
        guardrail.disable_symbol(symbol("MSFT"));

        const auto result = guardrail.check(
            make_enter_guardrail_order(enter(4, "MSFT", 100, 100'000))
        );

        assert(result.rejected());
        assert(result.reject_reason == GuardrailRejectReason::symbol_disabled);

        guardrail.enable_symbol(symbol("MSFT"));

        const auto allowed = guardrail.check(
            make_enter_guardrail_order(enter(5, "MSFT", 100, 100'000))
        );

        assert(allowed.allowed());
    }

    {
        guardrail.set_default_symbol_enabled(false);
        guardrail.enable_symbol(symbol("AAPL"));

        const auto allowed = guardrail.check(
            make_enter_guardrail_order(enter(6, "AAPL", 100, 100'000))
        );
        const auto rejected = guardrail.check(
            make_enter_guardrail_order(enter(7, "TSLA", 100, 100'000))
        );

        assert(allowed.allowed());
        assert(rejected.rejected());
        assert(rejected.reject_reason == GuardrailRejectReason::symbol_disabled);

        guardrail.set_default_symbol_enabled(true);
        guardrail.clear_symbol_overrides();
    }

    {
        guardrail.set_cancel_only(true);

        const auto enter_result = guardrail.check(
            make_enter_guardrail_order(enter(8, "AAPL", 100, 100'000))
        );
        const auto cancel_result = guardrail.check(
            make_cancel_guardrail_order(cancel(8, 0))
        );

        assert(enter_result.rejected());
        assert(enter_result.reject_reason == GuardrailRejectReason::cancel_only);

        assert(cancel_result.allowed());
        assert(cancel_result.kind == GuardrailOrderKind::cancel);

        guardrail.set_cancel_only(false);
    }

    {
        guardrail.set_global_halted(true);

        const auto enter_result = guardrail.check(
            make_enter_guardrail_order(enter(9, "AAPL", 100, 100'000))
        );
        const auto cancel_result = guardrail.check(
            make_cancel_guardrail_order(cancel(9, 0))
        );

        assert(enter_result.rejected());
        assert(enter_result.reject_reason == GuardrailRejectReason::global_halt);

        assert(cancel_result.rejected());
        assert(cancel_result.reject_reason == GuardrailRejectReason::global_halt);

        guardrail.set_global_halted(false);
    }

    {
        const auto replace_message = replace(1, 10, 100, 100'000);
        const auto order = make_replace_guardrail_order(
            replace_message,
            symbol("AAPL")
        );

        const auto result = guardrail.check(order);

        assert(result.allowed());
        assert(result.kind == GuardrailOrderKind::replace);
        assert(result.user_ref_num == 10);
    }

    {
        const GuardrailOrder order{
            .kind = GuardrailOrderKind::enter,
            .user_ref_num = 11,
            .symbol = {},
            .has_symbol = false,
            .quantity = 100,
            .price = 100'000
        };

        const auto result = guardrail.check(order);

        assert(result.rejected());
        assert(result.reject_reason == GuardrailRejectReason::symbol_missing);
    }

    {
        const auto result = guardrail.check(
            make_cancel_guardrail_order(cancel(12, 1'000'000))
        );

        assert(result.rejected());
        assert(result.reject_reason == GuardrailRejectReason::invalid_quantity);
    }

    return 0;
}