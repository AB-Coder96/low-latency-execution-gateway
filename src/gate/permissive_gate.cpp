#include "fgep/gate/permissive_gate.hpp"

namespace fgep::gate {

PermissiveGate::PermissiveGate(
    GuardrailState guardrail_state
) noexcept
    : guardrail_state_{std::move(guardrail_state)} {
}

GateSubmitResult PermissiveGate::submit(
    const GuardrailOrder& order,
    execution::ExecutionBackend& backend,
    const execution::BackendSubmitRequest& request
) const {
    const auto guardrail_result = guardrail_state_.check(order);

    if (guardrail_result.rejected()) {
        return reject_from_guardrail(guardrail_result, request);
    }

    return from_backend(
        guardrail_result,
        backend.submit(request)
    );
}

const GuardrailState& PermissiveGate::guardrail_state() const noexcept {
    return guardrail_state_;
}

GuardrailState& PermissiveGate::guardrail_state() noexcept {
    return guardrail_state_;
}

void PermissiveGate::set_guardrail_state(
    GuardrailState guardrail_state
) noexcept {
    guardrail_state_ = std::move(guardrail_state);
}

GateSubmitResult PermissiveGate::reject_from_guardrail(
    const GuardrailResult& guardrail_result,
    const execution::BackendSubmitRequest& request
) noexcept {
    return GateSubmitResult{
        .decision = GateSubmitDecision::rejected,
        .reject_source = GateRejectSource::guardrail,
        .guardrail_result = guardrail_result,
        .backend_result = execution::BackendSubmitResult{
            .decision = execution::BackendSubmitDecision::rejected,
            .reject_reason = execution::BackendRejectReason::none,
            .user_ref_num = request.user_ref_num,
            .kind = request.kind,
            .payload_size = request.payload.size()
        }
    };
}

GateSubmitResult PermissiveGate::from_backend(
    const GuardrailResult& guardrail_result,
    const execution::BackendSubmitResult& backend_result
) noexcept {
    return GateSubmitResult{
        .decision = backend_result.accepted()
            ? GateSubmitDecision::accepted
            : GateSubmitDecision::rejected,
        .reject_source = backend_result.accepted()
            ? GateRejectSource::none
            : GateRejectSource::backend,
        .guardrail_result = guardrail_result,
        .backend_result = backend_result
    };
}

execution::BackendOrderKind backend_order_kind_from_guardrail(
    GuardrailOrderKind kind
) noexcept {
    switch (kind) {
    case GuardrailOrderKind::enter:
        return execution::BackendOrderKind::enter;
    case GuardrailOrderKind::replace:
        return execution::BackendOrderKind::replace;
    case GuardrailOrderKind::cancel:
        return execution::BackendOrderKind::cancel;
    }

    return execution::BackendOrderKind::enter;
}

} // namespace fgep::gate