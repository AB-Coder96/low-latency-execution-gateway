#pragma once

#include "fgep/execution/order_lifecycle.hpp"
#include "fgep/risk/risk_supervisor.hpp"

#include <cstdint>

namespace fgep::execution {

enum class ExecutionAction : std::uint8_t {
    enter,
    replace,
    cancel,
    execute
};

enum class ExecutionDecision : std::uint8_t {
    accepted,
    rejected
};

enum class ExecutionRejectSource : std::uint8_t {
    none,
    risk,
    lifecycle
};

struct ExecutionResult {
    ExecutionAction action{ExecutionAction::enter};
    ExecutionDecision decision{ExecutionDecision::rejected};
    ExecutionRejectSource reject_source{ExecutionRejectSource::none};

    ouch::UserRefNum user_ref_num{};
    ouch::UserRefNum previous_user_ref_num{};

    ouch::Quantity decrement_quantity{};
    ouch::Quantity leaves_quantity{};

    risk::RiskRejectReason risk_reject_reason{risk::RiskRejectReason::none};
    LifecycleStatus lifecycle_status{LifecycleStatus::rejected};
    LifecycleRejectReason lifecycle_reject_reason{LifecycleRejectReason::none};

    [[nodiscard]] constexpr bool accepted() const noexcept {
        return decision == ExecutionDecision::accepted;
    }

    [[nodiscard]] constexpr bool rejected() const noexcept {
        return decision == ExecutionDecision::rejected;
    }
};

class SimulatedExecutionEngine {
public:
    SimulatedExecutionEngine() = default;

    explicit SimulatedExecutionEngine(
        risk::RiskSupervisor risk_supervisor
    ) noexcept;

    [[nodiscard]] ExecutionResult submit_enter(
        const ouch::EnterOrderMessage& message
    );

    [[nodiscard]] ExecutionResult submit_replace(
        const ouch::ReplaceOrderMessage& message
    );

    [[nodiscard]] ExecutionResult submit_cancel(
        const ouch::CancelOrderMessage& message
    );

    [[nodiscard]] ExecutionResult execute(
        ouch::UserRefNum user_ref_num,
        ouch::Quantity executed_quantity
    );

    [[nodiscard]] const LiveOrder* find(
        ouch::UserRefNum user_ref_num
    ) const noexcept;

    [[nodiscard]] bool contains(ouch::UserRefNum user_ref_num) const noexcept;

    [[nodiscard]] std::size_t live_order_count() const noexcept;

    [[nodiscard]] const risk::RiskSupervisor& risk() const noexcept;
    [[nodiscard]] risk::RiskSupervisor& risk() noexcept;

    [[nodiscard]] const OrderLifecycleEngine& lifecycle() const noexcept;

    void clear() noexcept;

private:
    risk::RiskSupervisor risk_{};
    OrderLifecycleEngine lifecycle_{};

    [[nodiscard]] static ExecutionResult from_risk_reject(
        ExecutionAction action,
        const risk::RiskResult& risk_result
    ) noexcept;

    [[nodiscard]] static ExecutionResult from_lifecycle(
        ExecutionAction action,
        const LifecycleResult& lifecycle_result
    ) noexcept;
};

} // namespace fgep::execution