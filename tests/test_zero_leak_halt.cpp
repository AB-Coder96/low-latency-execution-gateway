#include "fgep/gate/permissive_gate.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {

inline constexpr std::size_t post_halt_candidate_count{1'000'000};

[[nodiscard]] fgep::ouch::Symbol symbol(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<8>(text);
    assert(value.ok());
    return value.value;
}

[[nodiscard]] std::array<std::byte, 4> payload() noexcept {
    return {
        std::byte{0x48},
        std::byte{0x41},
        std::byte{0x4c},
        std::byte{0x54}
    };
}

[[nodiscard]] fgep::gate::GuardrailOrder enter_order(
    fgep::ouch::UserRefNum user_ref_num
) {
    return fgep::gate::GuardrailOrder{
        .kind = fgep::gate::GuardrailOrderKind::enter,
        .user_ref_num = user_ref_num,
        .symbol = symbol("AAPL"),
        .has_symbol = true,
        .quantity = 100,
        .price = 100'000
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

    GuardrailState state{GuardrailLimits{
        .max_order_quantity = 1'000,
        .max_order_notional = 50'000'000ULL
    }};

    PermissiveGate gate{state};
    RecordingExecutionBackend backend{};
    const auto bytes = payload();

    {
        const auto order = enter_order(1);
        const auto result = gate.submit(
            order,
            backend,
            request_for(order, bytes)
        );

        assert(result.accepted());
        assert(result.guardrail_result.allowed());
        assert(result.backend_result.accepted());
        assert(backend.submitted_count() == 1);
    }

    gate.guardrail_state().set_global_halted(true);

    std::size_t leaked_count = 0;
    std::size_t guardrail_reject_count = 0;
    std::size_t backend_submit_count_at_halt = backend.submitted_count();

    for (std::size_t index = 0; index < post_halt_candidate_count; ++index) {
        const auto user_ref_num = static_cast<fgep::ouch::UserRefNum>(
            index + 2U
        );
        const auto order = enter_order(user_ref_num);
        const auto result = gate.submit(
            order,
            backend,
            request_for(order, bytes)
        );

        if (result.accepted()) {
            ++leaked_count;
        }

        if (
            result.rejected()
            && result.reject_source == GateRejectSource::guardrail
            && result.guardrail_result.reject_reason
                == GuardrailRejectReason::global_halt
        ) {
            ++guardrail_reject_count;
        }
    }

    assert(leaked_count == 0);
    assert(guardrail_reject_count == post_halt_candidate_count);
    assert(backend.submitted_count() == backend_submit_count_at_halt);
    assert(backend.submitted_count() == 1);

    return 0;
}