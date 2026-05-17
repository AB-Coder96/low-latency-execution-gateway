#include "fgep/gate/guardrail_control.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cassert>

namespace {

[[nodiscard]] fgep::ouch::Symbol symbol(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<8>(text);
    assert(value.ok());
    return value.value;
}

[[nodiscard]] fgep::gate::GuardrailOrder enter_order(
    const char* stock,
    fgep::ouch::Quantity quantity,
    fgep::ouch::Price4 price
) {
    return fgep::gate::GuardrailOrder{
        .kind = fgep::gate::GuardrailOrderKind::enter,
        .user_ref_num = 1,
        .symbol = symbol(stock),
        .has_symbol = true,
        .quantity = quantity,
        .price = price
    };
}

} // namespace

int main() {
    using namespace fgep::gate;

    {
        GuardrailControlPath control{};
        GuardrailState state{};

        assert(control.empty());
        assert(control.pending_count() == 0);
        assert(control.next_sequence() == 1);

        const auto update = control.push_set_global_halt(true);

        assert(update.sequence == 1);
        assert(update.kind == GuardrailUpdateKind::set_global_halt);
        assert(update.bool_value);
        assert(!control.empty());
        assert(control.pending_count() == 1);
        assert(control.next_sequence() == 2);

        const auto result = control.apply_next(state);

        assert(result.applied());
        assert(result.sequence == 1);
        assert(state.global_halted());
        assert(control.empty());

        const auto stats = control.stats();

        assert(stats.staged_count == 1);
        assert(stats.applied_count == 1);
        assert(stats.dropped_stale_count == 0);
        assert(stats.last_applied_sequence == 1);
    }

    {
        GuardrailControlPath control{};
        GuardrailState state{GuardrailLimits{
            .max_order_quantity = 1'000,
            .max_order_notional = 50'000'000ULL
        }};

        control.push_set_cancel_only(true);
        control.push_disable_symbol(symbol("MSFT"));
        control.push_set_limits(GuardrailLimits{
            .max_order_quantity = 10,
            .max_order_notional = 1'000'000ULL
        });

        assert(control.pending_count() == 3);

        const auto applied = control.apply_all(state);

        assert(applied == 3);
        assert(control.empty());
        assert(state.cancel_only());
        assert(!state.is_symbol_enabled(symbol("MSFT")));
        assert(state.limits().max_order_quantity == 10);
        assert(state.limits().max_order_notional == 1'000'000ULL);

        const auto stats = control.stats();

        assert(stats.staged_count == 3);
        assert(stats.applied_count == 3);
        assert(stats.last_applied_sequence == 3);
    }

    {
        GuardrailControlPath control{};
        GuardrailState state{};

        control.push_set_default_symbol_enabled(false);
        control.push_enable_symbol(symbol("AAPL"));

        const auto applied = control.apply_all(state);

        assert(applied == 2);
        assert(!state.default_symbol_enabled());
        assert(state.is_symbol_enabled(symbol("AAPL")));
        assert(!state.is_symbol_enabled(symbol("MSFT")));

        control.push_clear_symbol_overrides();
        control.apply_all(state);

        assert(!state.default_symbol_enabled());
        assert(!state.is_symbol_enabled(symbol("AAPL")));
    }

    {
        GuardrailControlPath control{};
        GuardrailState state{};

        GuardrailUpdate stale{
            .sequence = 1,
            .kind = GuardrailUpdateKind::set_global_halt,
            .bool_value = false,
            .limits = {},
            .symbol = {}
        };

        control.push_set_global_halt(true);
        control.apply_all(state);

        assert(state.global_halted());

        control.enqueue(stale);

        const auto result = control.apply_next(state);

        assert(result.dropped());
        assert(result.sequence == 1);
        assert(state.global_halted());

        const auto stats = control.stats();

        assert(stats.dropped_stale_count == 1);
        assert(stats.applied_count == 1);
    }

    {
        GuardrailControlPath control{};
        GuardrailState state{};

        const auto result = control.apply_next(state);

        assert(result.decision == GuardrailApplyDecision::no_pending);
        assert(!result.applied());
        assert(!result.dropped());
    }

    {
        GuardrailControlPath control{};
        GuardrailState state{GuardrailLimits{
            .max_order_quantity = 100,
            .max_order_notional = 10'000'000ULL
        }};

        control.push_set_limits(GuardrailLimits{
            .max_order_quantity = 50,
            .max_order_notional = 5'000'000ULL
        });
        control.apply_all(state);

        const auto allowed = state.check(enter_order("AAPL", 50, 100'000));
        const auto rejected = state.check(enter_order("AAPL", 51, 100'000));

        assert(allowed.allowed());
        assert(rejected.rejected());
        assert(
            rejected.reject_reason
            == GuardrailRejectReason::max_order_quantity
        );

        control.reset();

        assert(control.empty());
        assert(control.next_sequence() == 1);
        assert(control.stats().staged_count == 0);
        assert(control.stats().applied_count == 0);
    }

    return 0;
}