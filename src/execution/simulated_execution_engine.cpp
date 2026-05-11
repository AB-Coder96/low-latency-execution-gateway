#include "fgep/execution/simulated_execution_engine.hpp"

namespace fgep::execution {

SimulatedExecutionEngine::SimulatedExecutionEngine(
    risk::RiskSupervisor risk_supervisor
) noexcept
    : risk_{risk_supervisor} {
}

ExecutionResult SimulatedExecutionEngine::submit_enter(
    const ouch::EnterOrderMessage& message
) {
    const auto risk_result = risk_.check_enter(message);

    if (risk_result.rejected()) {
        return from_risk_reject(ExecutionAction::enter, risk_result);
    }

    return from_lifecycle(
        ExecutionAction::enter,
        lifecycle_.enter(message)
    );
}

ExecutionResult SimulatedExecutionEngine::submit_replace(
    const ouch::ReplaceOrderMessage& message
) {
    const auto* current_order = lifecycle_.find(message.orig_user_ref_num);

    if (current_order == nullptr) {
        return from_lifecycle(
            ExecutionAction::replace,
            lifecycle_.replace(message)
        );
    }

    const auto risk_result = risk_.check_replace(
        message,
        current_order->symbol
    );

    if (risk_result.rejected()) {
        return from_risk_reject(ExecutionAction::replace, risk_result);
    }

    return from_lifecycle(
        ExecutionAction::replace,
        lifecycle_.replace(message)
    );
}

ExecutionResult SimulatedExecutionEngine::submit_cancel(
    const ouch::CancelOrderMessage& message
) {
    const auto risk_result = risk_.check_cancel(message);

    if (risk_result.rejected()) {
        return from_risk_reject(ExecutionAction::cancel, risk_result);
    }

    return from_lifecycle(
        ExecutionAction::cancel,
        lifecycle_.cancel(message)
    );
}

ExecutionResult SimulatedExecutionEngine::execute(
    ouch::UserRefNum user_ref_num,
    ouch::Quantity executed_quantity
) {
    return from_lifecycle(
        ExecutionAction::execute,
        lifecycle_.execute(user_ref_num, executed_quantity)
    );
}

const LiveOrder* SimulatedExecutionEngine::find(
    ouch::UserRefNum user_ref_num
) const noexcept {
    return lifecycle_.find(user_ref_num);
}

bool SimulatedExecutionEngine::contains(
    ouch::UserRefNum user_ref_num
) const noexcept {
    return lifecycle_.contains(user_ref_num);
}

std::size_t SimulatedExecutionEngine::live_order_count() const noexcept {
    return lifecycle_.live_order_count();
}

const risk::RiskSupervisor& SimulatedExecutionEngine::risk() const noexcept {
    return risk_;
}

risk::RiskSupervisor& SimulatedExecutionEngine::risk() noexcept {
    return risk_;
}

const OrderLifecycleEngine& SimulatedExecutionEngine::lifecycle()
    const noexcept {
    return lifecycle_;
}

void SimulatedExecutionEngine::clear() noexcept {
    lifecycle_.clear();
}

ExecutionResult SimulatedExecutionEngine::from_risk_reject(
    ExecutionAction action,
    const risk::RiskResult& risk_result
) noexcept {
    return ExecutionResult{
        .action = action,
        .decision = ExecutionDecision::rejected,
        .reject_source = ExecutionRejectSource::risk,
        .user_ref_num = risk_result.user_ref_num,
        .previous_user_ref_num = 0,
        .decrement_quantity = 0,
        .leaves_quantity = 0,
        .risk_reject_reason = risk_result.reject_reason,
        .lifecycle_status = LifecycleStatus::rejected,
        .lifecycle_reject_reason = LifecycleRejectReason::none
    };
}

ExecutionResult SimulatedExecutionEngine::from_lifecycle(
    ExecutionAction action,
    const LifecycleResult& lifecycle_result
) noexcept {
    return ExecutionResult{
        .action = action,
        .decision = lifecycle_result.accepted()
            ? ExecutionDecision::accepted
            : ExecutionDecision::rejected,
        .reject_source = lifecycle_result.accepted()
            ? ExecutionRejectSource::none
            : ExecutionRejectSource::lifecycle,
        .user_ref_num = lifecycle_result.user_ref_num,
        .previous_user_ref_num = lifecycle_result.previous_user_ref_num,
        .decrement_quantity = lifecycle_result.decrement_quantity,
        .leaves_quantity = lifecycle_result.leaves_quantity,
        .risk_reject_reason = risk::RiskRejectReason::none,
        .lifecycle_status = lifecycle_result.status,
        .lifecycle_reject_reason = lifecycle_result.reject_reason
    };
}

} // namespace fgep::execution