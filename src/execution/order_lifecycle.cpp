#include "fgep/execution/order_lifecycle.hpp"

namespace fgep::execution {
namespace {

[[nodiscard]] LifecycleResult reject(
    ouch::UserRefNum user_ref_num,
    LifecycleRejectReason reason
) noexcept {
    return LifecycleResult{
        .status = LifecycleStatus::rejected,
        .reject_reason = reason,
        .user_ref_num = user_ref_num,
        .previous_user_ref_num = 0,
        .decrement_quantity = 0,
        .leaves_quantity = 0
    };
}

} // namespace

LifecycleResult OrderLifecycleEngine::enter(
    const ouch::EnterOrderMessage& message
) {
    if (!ouch::is_valid_enter_order_message(message)) {
        if (!ouch::is_valid_enter_or_replace_quantity(message.quantity)) {
            return reject(
                message.user_ref_num,
                LifecycleRejectReason::invalid_quantity
            );
        }

        if (!ouch::is_valid_price4(message.price)) {
            return reject(
                message.user_ref_num,
                LifecycleRejectReason::invalid_price
            );
        }

        return reject(
            message.user_ref_num,
            LifecycleRejectReason::invalid_state_transition
        );
    }

    if (live_orders_.contains(message.user_ref_num)) {
        return reject(
            message.user_ref_num,
            LifecycleRejectReason::duplicate_user_ref_num
        );
    }

    if (is_stale_user_ref(message.user_ref_num)) {
        return reject(
            message.user_ref_num,
            LifecycleRejectReason::stale_user_ref_num
        );
    }

    LiveOrder order{
        .user_ref_num = message.user_ref_num,
        .side = message.side,
        .original_quantity = message.quantity,
        .leaves_quantity = message.quantity,
        .symbol = message.symbol,
        .price = message.price,
        .time_in_force = message.time_in_force,
        .display = message.display,
        .capacity = message.capacity,
        .intermarket_sweep_eligibility =
            message.intermarket_sweep_eligibility,
        .cross_type = message.cross_type,
        .cl_ord_id = message.cl_ord_id
    };

    live_orders_.emplace(message.user_ref_num, order);
    remember_user_ref(message.user_ref_num);

    return LifecycleResult{
        .status = LifecycleStatus::accepted,
        .reject_reason = LifecycleRejectReason::none,
        .user_ref_num = message.user_ref_num,
        .previous_user_ref_num = 0,
        .decrement_quantity = 0,
        .leaves_quantity = message.quantity
    };
}

LifecycleResult OrderLifecycleEngine::replace(
    const ouch::ReplaceOrderMessage& message
) {
    if (!ouch::is_valid_replace_order_message(message)) {
        if (!ouch::is_valid_enter_or_replace_quantity(message.quantity)) {
            return reject(
                message.user_ref_num,
                LifecycleRejectReason::invalid_quantity
            );
        }

        if (!ouch::is_valid_price4(message.price)) {
            return reject(
                message.user_ref_num,
                LifecycleRejectReason::invalid_price
            );
        }

        return reject(
            message.user_ref_num,
            LifecycleRejectReason::invalid_state_transition
        );
    }

    const auto original = live_orders_.find(message.orig_user_ref_num);

    if (original == live_orders_.end()) {
        return reject(message.user_ref_num, LifecycleRejectReason::unknown_order);
    }

    if (live_orders_.contains(message.user_ref_num)) {
        return reject(
            message.user_ref_num,
            LifecycleRejectReason::duplicate_user_ref_num
        );
    }

    if (is_stale_user_ref(message.user_ref_num)) {
        return reject(
            message.user_ref_num,
            LifecycleRejectReason::stale_user_ref_num
        );
    }

    const auto old_order = original->second;
    live_orders_.erase(original);

    LiveOrder replacement{
        .user_ref_num = message.user_ref_num,
        .side = old_order.side,
        .original_quantity = message.quantity,
        .leaves_quantity = message.quantity,
        .symbol = old_order.symbol,
        .price = message.price,
        .time_in_force = message.time_in_force,
        .display = message.display,
        .capacity = old_order.capacity,
        .intermarket_sweep_eligibility =
            message.intermarket_sweep_eligibility,
        .cross_type = old_order.cross_type,
        .cl_ord_id = message.cl_ord_id
    };

    live_orders_.emplace(message.user_ref_num, replacement);
    remember_user_ref(message.user_ref_num);

    return LifecycleResult{
        .status = LifecycleStatus::replaced,
        .reject_reason = LifecycleRejectReason::none,
        .user_ref_num = message.user_ref_num,
        .previous_user_ref_num = message.orig_user_ref_num,
        .decrement_quantity = old_order.leaves_quantity,
        .leaves_quantity = message.quantity
    };
}

LifecycleResult OrderLifecycleEngine::cancel(
    const ouch::CancelOrderMessage& message
) {
    if (!ouch::is_valid_cancel_order_message(message)) {
        return reject(
            message.user_ref_num,
            LifecycleRejectReason::invalid_quantity
        );
    }

    const auto order = live_orders_.find(message.user_ref_num);

    if (order == live_orders_.end()) {
        return reject(message.user_ref_num, LifecycleRejectReason::unknown_order);
    }

    const auto old_leaves = order->second.leaves_quantity;

    if (message.quantity == 0 || message.quantity >= old_leaves) {
        live_orders_.erase(order);

        return LifecycleResult{
            .status = LifecycleStatus::canceled,
            .reject_reason = LifecycleRejectReason::none,
            .user_ref_num = message.user_ref_num,
            .previous_user_ref_num = 0,
            .decrement_quantity = old_leaves,
            .leaves_quantity = 0
        };
    }

    order->second.leaves_quantity -= message.quantity;

    return LifecycleResult{
        .status = LifecycleStatus::partially_canceled,
        .reject_reason = LifecycleRejectReason::none,
        .user_ref_num = message.user_ref_num,
        .previous_user_ref_num = 0,
        .decrement_quantity = message.quantity,
        .leaves_quantity = order->second.leaves_quantity
    };
}

LifecycleResult OrderLifecycleEngine::execute(
    ouch::UserRefNum user_ref_num,
    ouch::Quantity executed_quantity
) {
    if (!ouch::is_valid_enter_or_replace_quantity(executed_quantity)) {
        return reject(user_ref_num, LifecycleRejectReason::invalid_quantity);
    }

    const auto order = live_orders_.find(user_ref_num);

    if (order == live_orders_.end()) {
        return reject(user_ref_num, LifecycleRejectReason::unknown_order);
    }

    if (executed_quantity > order->second.leaves_quantity) {
        return reject(user_ref_num, LifecycleRejectReason::overfill);
    }

    order->second.leaves_quantity -= executed_quantity;

    if (order->second.leaves_quantity == 0) {
        live_orders_.erase(order);

        return LifecycleResult{
            .status = LifecycleStatus::filled,
            .reject_reason = LifecycleRejectReason::none,
            .user_ref_num = user_ref_num,
            .previous_user_ref_num = 0,
            .decrement_quantity = executed_quantity,
            .leaves_quantity = 0
        };
    }

    return LifecycleResult{
        .status = LifecycleStatus::partially_filled,
        .reject_reason = LifecycleRejectReason::none,
        .user_ref_num = user_ref_num,
        .previous_user_ref_num = 0,
        .decrement_quantity = executed_quantity,
        .leaves_quantity = order->second.leaves_quantity
    };
}

const LiveOrder* OrderLifecycleEngine::find(
    ouch::UserRefNum user_ref_num
) const noexcept {
    const auto order = live_orders_.find(user_ref_num);

    if (order == live_orders_.end()) {
        return nullptr;
    }

    return &order->second;
}

bool OrderLifecycleEngine::contains(
    ouch::UserRefNum user_ref_num
) const noexcept {
    return live_orders_.contains(user_ref_num);
}

std::size_t OrderLifecycleEngine::live_order_count() const noexcept {
    return live_orders_.size();
}

ouch::UserRefNum OrderLifecycleEngine::highest_user_ref_num() const noexcept {
    return highest_user_ref_num_;
}

void OrderLifecycleEngine::clear() noexcept {
    live_orders_.clear();
    highest_user_ref_num_ = 0;
}

bool OrderLifecycleEngine::is_stale_user_ref(
    ouch::UserRefNum user_ref_num
) const noexcept {
    return user_ref_num <= highest_user_ref_num_;
}

void OrderLifecycleEngine::remember_user_ref(
    ouch::UserRefNum user_ref_num
) noexcept {
    if (user_ref_num > highest_user_ref_num_) {
        highest_user_ref_num_ = user_ref_num;
    }
}

} // namespace fgep::execution