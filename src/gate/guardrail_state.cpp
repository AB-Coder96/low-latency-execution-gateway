#include "fgep/gate/guardrail_state.hpp"

#include <limits>

namespace fgep::gate {

GuardrailState::GuardrailState(GuardrailLimits limits) noexcept
    : limits_{limits} {
}

GuardrailResult GuardrailState::check(
    const GuardrailOrder& order
) const noexcept {
    if (global_halted_) {
        return reject(order, GuardrailRejectReason::global_halt);
    }

    if (cancel_only_ && !is_cancel(order.kind)) {
        return reject(order, GuardrailRejectReason::cancel_only);
    }

    if (!is_cancel(order.kind)) {
        if (!ouch::is_valid_enter_or_replace_quantity(order.quantity)) {
            return reject(order, GuardrailRejectReason::invalid_quantity);
        }

        if (!ouch::is_valid_price4(order.price)) {
            return reject(order, GuardrailRejectReason::invalid_price);
        }

        if (requires_symbol(order.kind) && !order.has_symbol) {
            return reject(order, GuardrailRejectReason::symbol_missing);
        }

        if (order.has_symbol && !is_symbol_enabled(order.symbol)) {
            return reject(order, GuardrailRejectReason::symbol_disabled);
        }

        if (order.quantity > limits_.max_order_quantity) {
            return reject(order, GuardrailRejectReason::max_order_quantity);
        }

        if (exceeds_notional_limit(order.quantity, order.price)) {
            return reject(
                order,
                GuardrailRejectReason::max_order_notional,
                calculate_notional(order.quantity, order.price)
            );
        }

        return allow(
            order,
            calculate_notional(order.quantity, order.price)
        );
    }

    if (!ouch::is_valid_cancel_quantity(order.quantity)) {
        return reject(order, GuardrailRejectReason::invalid_quantity);
    }

    return allow(order, 0);
}

void GuardrailState::set_limits(GuardrailLimits limits) noexcept {
    limits_ = limits;
}

GuardrailLimits GuardrailState::limits() const noexcept {
    return limits_;
}

void GuardrailState::set_global_halted(bool halted) noexcept {
    global_halted_ = halted;
}

bool GuardrailState::global_halted() const noexcept {
    return global_halted_;
}

void GuardrailState::set_cancel_only(bool enabled) noexcept {
    cancel_only_ = enabled;
}

bool GuardrailState::cancel_only() const noexcept {
    return cancel_only_;
}

void GuardrailState::set_default_symbol_enabled(bool enabled) noexcept {
    default_symbol_enabled_ = enabled;
}

bool GuardrailState::default_symbol_enabled() const noexcept {
    return default_symbol_enabled_;
}

void GuardrailState::enable_symbol(const ouch::Symbol& symbol) {
    disabled_symbols_.erase(symbol);
    enabled_symbols_.insert(symbol);
}

void GuardrailState::disable_symbol(const ouch::Symbol& symbol) {
    enabled_symbols_.erase(symbol);
    disabled_symbols_.insert(symbol);
}

void GuardrailState::clear_symbol_overrides() {
    enabled_symbols_.clear();
    disabled_symbols_.clear();
}

bool GuardrailState::is_symbol_enabled(
    const ouch::Symbol& symbol
) const noexcept {
    if (disabled_symbols_.contains(symbol)) {
        return false;
    }

    if (enabled_symbols_.contains(symbol)) {
        return true;
    }

    return default_symbol_enabled_;
}

std::size_t GuardrailState::SymbolHash::operator()(
    const ouch::Symbol& symbol
) const noexcept {
    std::size_t hash = 0;

    for (const char character : symbol) {
        hash = (hash * 131U)
            ^ static_cast<std::size_t>(
                static_cast<unsigned char>(character)
            );
    }

    return hash;
}

GuardrailResult GuardrailState::allow(
    const GuardrailOrder& order,
    Notional notional
) const noexcept {
    return GuardrailResult{
        .decision = GuardrailDecision::allowed,
        .reject_reason = GuardrailRejectReason::none,
        .kind = order.kind,
        .user_ref_num = order.user_ref_num,
        .quantity = order.quantity,
        .price = order.price,
        .notional = notional
    };
}

GuardrailResult GuardrailState::reject(
    const GuardrailOrder& order,
    GuardrailRejectReason reason,
    Notional notional
) const noexcept {
    return GuardrailResult{
        .decision = GuardrailDecision::rejected,
        .reject_reason = reason,
        .kind = order.kind,
        .user_ref_num = order.user_ref_num,
        .quantity = order.quantity,
        .price = order.price,
        .notional = notional
    };
}

bool GuardrailState::requires_symbol(
    GuardrailOrderKind kind
) const noexcept {
    return kind == GuardrailOrderKind::enter
        || kind == GuardrailOrderKind::replace;
}

bool GuardrailState::is_cancel(
    GuardrailOrderKind kind
) const noexcept {
    return kind == GuardrailOrderKind::cancel;
}

Notional GuardrailState::calculate_notional(
    ouch::Quantity quantity,
    ouch::Price4 price
) const noexcept {
    if (quantity == 0 || price == 0) {
        return 0;
    }

    if (price > std::numeric_limits<Notional>::max() / quantity) {
        return std::numeric_limits<Notional>::max();
    }

    return static_cast<Notional>(price) * quantity;
}

bool GuardrailState::exceeds_notional_limit(
    ouch::Quantity quantity,
    ouch::Price4 price
) const noexcept {
    if (quantity == 0 || price == 0) {
        return false;
    }

    if (price > std::numeric_limits<Notional>::max() / quantity) {
        return true;
    }

    return calculate_notional(quantity, price) > limits_.max_order_notional;
}

GuardrailOrder make_enter_guardrail_order(
    const ouch::EnterOrderMessage& message
) noexcept {
    return GuardrailOrder{
        .kind = GuardrailOrderKind::enter,
        .user_ref_num = message.user_ref_num,
        .symbol = message.symbol,
        .has_symbol = true,
        .quantity = message.quantity,
        .price = message.price
    };
}

GuardrailOrder make_replace_guardrail_order(
    const ouch::ReplaceOrderMessage& message,
    const ouch::Symbol& current_symbol
) noexcept {
    return GuardrailOrder{
        .kind = GuardrailOrderKind::replace,
        .user_ref_num = message.user_ref_num,
        .symbol = current_symbol,
        .has_symbol = true,
        .quantity = message.quantity,
        .price = message.price
    };
}

GuardrailOrder make_cancel_guardrail_order(
    const ouch::CancelOrderMessage& message
) noexcept {
    return GuardrailOrder{
        .kind = GuardrailOrderKind::cancel,
        .user_ref_num = message.user_ref_num,
        .symbol = {},
        .has_symbol = false,
        .quantity = message.quantity,
        .price = 0
    };
}

} // namespace fgep::gate