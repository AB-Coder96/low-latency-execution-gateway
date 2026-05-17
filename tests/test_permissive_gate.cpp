#include "fgep/gate/permissive_gate.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <array>
#include <cassert>
#include <cstddef>

namespace {

[[nodiscard]] fgep::ouch::Symbol symbol(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<8>(text);
    assert(value.ok());
    return value.value;
}

[[nodiscard]] std::array<std::byte, 4> payload() noexcept {
    return {
        std::byte{0x47},
        std::byte{0x41},
        std::byte{0x54},
        std::byte{0x45}
    };
}

[[nodiscard]] fgep::gate::GuardrailOrder enter_order(
    fgep::ouch::UserRefNum user_ref_num,
    const char* stock,
    fgep::ouch::Quantity quantity,
    fgep::ouch::Price4 price
) {
    return fgep::gate::GuardrailOrder{
        .kind = fgep::gate::GuardrailOrderKind::enter,
        .user_ref_num = user_ref_num,
        .symbol = symbol(stock),
        .has_symbol = true,
        .quantity = quantity,
        .price = price
    };
}

[[nodiscard]] fgep::gate::GuardrailOrder cancel_order(
    fgep::ouch::UserRefNum user_ref_num,
    fgep::ouch::Quantity quantity
) {
    return fgep::gate::GuardrailOrder{
        .kind = fgep::gate::GuardrailOrderKind::cancel,
        .user_ref_num = user_ref_num,
        .symbol = {},
        .has_symbol = false,
        .quantity = quantity,
        .price = 0
    };
}

[[nodiscard]] fgep::execution::BackendSubmitRequest request_for(
    const fgep::gate::GuardrailOrder& order,
    std::span<const std::byte> bytes
) noexcept {
    return fgep::execution::BackendSubmitRequest{
        .user_ref_num = order.user_ref_num,
        .kind = fgep::gate::backend_order_kind_from_guardrail(order.kind),
        .payload = bytes
    };
}

} // namespace

int main() {
    using namespace fgep::gate;
    using namespace fgep::execution;

    {
        GuardrailState state{GuardrailLimits{
            .max_order_quantity = 1'000,
            .max_order_notional = 50'000'000ULL
        }};
        PermissiveGate gate{state};
        RecordingExecutionBackend backend{};

        const auto bytes = payload();
        const auto order = enter_order(1, "AAPL", 100, 100'000);
        const auto result = gate.submit(
            order,
            backend,
            request_for(order, bytes)
        );

        assert(result.accepted());
        assert(result.reject_source == GateRejectSource::none);
        assert(result.guardrail_result.allowed());
        assert(result.backend_result.accepted());
        assert(backend.submitted_count() == 1);
        assert(backend.submissions()[0].user_ref_num == 1);
        assert(backend.submissions()[0].kind == BackendOrderKind::enter);
    }

    {
        GuardrailState state{GuardrailLimits{
            .max_order_quantity = 1'000,
            .max_order_notional = 50'000'000ULL
        }};
        PermissiveGate gate{state};
        RecordingExecutionBackend backend{};

        const auto bytes = payload();
        const auto order = enter_order(2, "AAPL", 1'001, 100'000);
        const auto result = gate.submit(
            order,
            backend,
            request_for(order, bytes)
        );

        assert(result.rejected());
        assert(result.reject_source == GateRejectSource::guardrail);
        assert(result.guardrail_result.rejected());
        assert(
            result.guardrail_result.reject_reason
            == GuardrailRejectReason::max_order_quantity
        );
        assert(backend.empty());
    }

    {
        GuardrailState state{GuardrailLimits{
            .max_order_quantity = 1'000,
            .max_order_notional = 50'000'000ULL
        }};
        state.disable_symbol(symbol("MSFT"));

        PermissiveGate gate{state};
        RecordingExecutionBackend backend{};

        const auto bytes = payload();
        const auto order = enter_order(3, "MSFT", 100, 100'000);
        const auto result = gate.submit(
            order,
            backend,
            request_for(order, bytes)
        );

        assert(result.rejected());
        assert(result.reject_source == GateRejectSource::guardrail);
        assert(
            result.guardrail_result.reject_reason
            == GuardrailRejectReason::symbol_disabled
        );
        assert(backend.empty());
    }

    {
        GuardrailState state{};
        state.set_cancel_only(true);

        PermissiveGate gate{state};
        RecordingExecutionBackend backend{};

        const auto bytes = payload();

        const auto enter = enter_order(4, "AAPL", 100, 100'000);
        const auto enter_result = gate.submit(
            enter,
            backend,
            request_for(enter, bytes)
        );

        assert(enter_result.rejected());
        assert(enter_result.reject_source == GateRejectSource::guardrail);
        assert(
            enter_result.guardrail_result.reject_reason
            == GuardrailRejectReason::cancel_only
        );
        assert(backend.empty());

        const auto cancel = cancel_order(4, 0);
        const auto cancel_result = gate.submit(
            cancel,
            backend,
            request_for(cancel, bytes)
        );

        assert(cancel_result.accepted());
        assert(cancel_result.guardrail_result.allowed());
        assert(cancel_result.backend_result.accepted());
        assert(backend.submitted_count() == 1);
        assert(backend.submissions()[0].kind == BackendOrderKind::cancel);
    }

    {
        GuardrailState state{};
        state.set_global_halted(true);

        PermissiveGate gate{state};
        RecordingExecutionBackend backend{};

        const auto bytes = payload();

        const auto order = cancel_order(5, 0);
        const auto result = gate.submit(
            order,
            backend,
            request_for(order, bytes)
        );

        assert(result.rejected());
        assert(result.reject_source == GateRejectSource::guardrail);
        assert(
            result.guardrail_result.reject_reason
            == GuardrailRejectReason::global_halt
        );
        assert(backend.empty());
    }

    {
        GuardrailState state{};
        PermissiveGate gate{state};
        RecordingExecutionBackend backend{};
        backend.set_open(false);

        const auto bytes = payload();
        const auto order = enter_order(6, "AAPL", 100, 100'000);
        const auto result = gate.submit(
            order,
            backend,
            request_for(order, bytes)
        );

        assert(result.rejected());
        assert(result.reject_source == GateRejectSource::backend);
        assert(result.guardrail_result.allowed());
        assert(result.backend_result.rejected());
        assert(
            result.backend_result.reject_reason
            == BackendRejectReason::backend_closed
        );
    }

    {
        assert(
            backend_order_kind_from_guardrail(GuardrailOrderKind::enter)
            == BackendOrderKind::enter
        );
        assert(
            backend_order_kind_from_guardrail(GuardrailOrderKind::replace)
            == BackendOrderKind::replace
        );
        assert(
            backend_order_kind_from_guardrail(GuardrailOrderKind::cancel)
            == BackendOrderKind::cancel
        );
    }

    return 0;
}