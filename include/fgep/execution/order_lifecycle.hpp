#pragma once

#include "fgep/ouch/ouch_wire_messages.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace fgep::execution {

enum class LifecycleStatus : std::uint8_t {
    accepted,
    replaced,
    canceled,
    partially_canceled,
    partially_filled,
    filled,
    rejected
};

enum class LifecycleRejectReason : std::uint8_t {
    none,
    duplicate_user_ref_num,
    stale_user_ref_num,
    unknown_order,
    invalid_quantity,
    invalid_price,
    invalid_state_transition,
    overfill
};

struct LiveOrder {
    ouch::UserRefNum user_ref_num{};
    ouch::Side side{ouch::Side::buy};
    ouch::Quantity original_quantity{};
    ouch::Quantity leaves_quantity{};
    ouch::Symbol symbol{};
    ouch::Price4 price{};
    ouch::TimeInForce time_in_force{ouch::TimeInForce::day};
    ouch::Display display{ouch::Display::visible};
    ouch::Capacity capacity{ouch::Capacity::agency};
    ouch::IntermarketSweepEligibility intermarket_sweep_eligibility{
        ouch::IntermarketSweepEligibility::not_eligible
    };
    ouch::CrossType cross_type{ouch::CrossType::continuous_market};
    ouch::ClOrdId cl_ord_id{};
};

struct LifecycleResult {
    LifecycleStatus status{LifecycleStatus::rejected};
    LifecycleRejectReason reject_reason{LifecycleRejectReason::none};
    ouch::UserRefNum user_ref_num{};
    ouch::UserRefNum previous_user_ref_num{};
    ouch::Quantity decrement_quantity{};
    ouch::Quantity leaves_quantity{};

    [[nodiscard]] constexpr bool accepted() const noexcept {
        return status != LifecycleStatus::rejected;
    }

    [[nodiscard]] constexpr bool rejected() const noexcept {
        return status == LifecycleStatus::rejected;
    }
};

class OrderLifecycleEngine {
public:
    [[nodiscard]] LifecycleResult enter(
        const ouch::EnterOrderMessage& message
    );

    [[nodiscard]] LifecycleResult replace(
        const ouch::ReplaceOrderMessage& message
    );

    [[nodiscard]] LifecycleResult cancel(
        const ouch::CancelOrderMessage& message
    );

    [[nodiscard]] LifecycleResult execute(
        ouch::UserRefNum user_ref_num,
        ouch::Quantity executed_quantity
    );

    [[nodiscard]] const LiveOrder* find(
        ouch::UserRefNum user_ref_num
    ) const noexcept;

    [[nodiscard]] bool contains(ouch::UserRefNum user_ref_num) const noexcept;

    [[nodiscard]] std::size_t live_order_count() const noexcept;

    [[nodiscard]] ouch::UserRefNum highest_user_ref_num() const noexcept;

    void clear() noexcept;

private:
    std::unordered_map<ouch::UserRefNum, LiveOrder> live_orders_{};
    ouch::UserRefNum highest_user_ref_num_{};

    [[nodiscard]] bool is_stale_user_ref(
        ouch::UserRefNum user_ref_num
    ) const noexcept;

    void remember_user_ref(ouch::UserRefNum user_ref_num) noexcept;
};

} // namespace fgep::execution