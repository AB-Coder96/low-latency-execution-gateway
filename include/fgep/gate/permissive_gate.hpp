#pragma once

#include "fgep/execution/execution_backend.hpp"
#include "fgep/gate/guardrail_state.hpp"

#include <cstdint>

namespace fgep::gate {

enum class GateSubmitDecision : std::uint8_t {
    accepted,
    rejected
};

enum class GateRejectSource : std::uint8_t {
    none,
    guardrail,
    backend
};

struct GateSubmitResult {
    GateSubmitDecision decision{GateSubmitDecision::rejected};
    GateRejectSource reject_source{GateRejectSource::none};

    GuardrailResult guardrail_result{};
    execution::BackendSubmitResult backend_result{};

    [[nodiscard]] constexpr bool accepted() const noexcept {
        return decision == GateSubmitDecision::accepted;
    }

    [[nodiscard]] constexpr bool rejected() const noexcept {
        return decision == GateSubmitDecision::rejected;
    }
};

class PermissiveGate {
public:
    PermissiveGate() = default;

    explicit PermissiveGate(GuardrailState guardrail_state) noexcept;

    [[nodiscard]] GateSubmitResult submit(
        const GuardrailOrder& order,
        execution::ExecutionBackend& backend,
        const execution::BackendSubmitRequest& request
    ) const;

    [[nodiscard]] const GuardrailState& guardrail_state() const noexcept;
    [[nodiscard]] GuardrailState& guardrail_state() noexcept;

    void set_guardrail_state(GuardrailState guardrail_state) noexcept;

private:
    GuardrailState guardrail_state_{};

    [[nodiscard]] static GateSubmitResult reject_from_guardrail(
        const GuardrailResult& guardrail_result,
        const execution::BackendSubmitRequest& request
    ) noexcept;

    [[nodiscard]] static GateSubmitResult from_backend(
        const GuardrailResult& guardrail_result,
        const execution::BackendSubmitResult& backend_result
    ) noexcept;
};

[[nodiscard]] execution::BackendOrderKind backend_order_kind_from_guardrail(
    GuardrailOrderKind kind
) noexcept;

} // namespace fgep::gate