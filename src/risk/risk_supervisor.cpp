#include "fgep/risk/risk_supervisor.hpp"

#include <limits>

namespace fgep::risk {

RiskSupervisor::RiskSupervisor(RiskLimits limits) noexcept
    : limits_{limits} {
}

RiskResult RiskSupervisor::check_enter(
    const ouch::EnterOrderMessage& message
) const noexcept {
    return check_common_order_risk(
        message.user_ref_num,
        message.quantity,
        message.price,
        message.symbol
    );
}

RiskResult RiskSupervisor::check_replace(
    const ouch::ReplaceOrderMessage& message,
    const ouch::Symbol& current_symbol
) const noexcept {
    return check_common_order_risk(
        message.user_ref_num,
        message.quantity,
        message.price,
        current_symbol
    );
}

RiskResult RiskSupervisor::check_cancel(
    const ouch::CancelOrderMessage& message
) const noexcept {
    if (global_halted_) {
        return reject(
            RiskRejectReason::global_halt,
            message.user_ref_num,
            message.quantity,
            0
        );
    }

    if (!ouch::is_valid_cancel_quantity(message.quantity)) {
        return reject(
            RiskRejectReason::invalid_quantity,
            message.user_ref_num,
            message.quantity,
            0
        );
    }

    return accept(message.user_ref_num, message.quantity, 0, 0);
}

void RiskSupervisor::set_limits(RiskLimits limits) noexcept {
    limits_ = limits;
}

RiskLimits RiskSupervisor::limits() const noexcept {
    return limits_;
}

void RiskSupervisor::set_global_halted(bool halted) noexcept {
    global_halted_ = halted;
}

bool RiskSupervisor::global_halted() const noexcept {
    return global_halted_;
}

void RiskSupervisor::set_cancel_only(bool enabled) noexcept {
    cancel_only_ = enabled;
}

bool RiskSupervisor::cancel_only() const noexcept {
    return cancel_only_;
}

void RiskSupervisor::set_default_symbol_enabled(bool enabled) noexcept {
    default_symbol_enabled_ = enabled;
}

bool RiskSupervisor::default_symbol_enabled() const noexcept {
    return default_symbol_enabled_;
}

void RiskSupervisor::enable_symbol(const ouch::Symbol& symbol) {
    disabled_symbols_.erase(symbol);
    enabled_symbols_.insert(symbol);
}

void RiskSupervisor::disable_symbol(const ouch::Symbol& symbol) {
    enabled_symbols_.erase(symbol);
    disabled_symbols_.insert(symbol);
}

bool RiskSupervisor::is_symbol_enabled(
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

void RiskSupervisor::clear_symbol_overrides() {
    enabled_symbols_.clear();
    disabled_symbols_.clear();
}

std::size_t RiskSupervisor::SymbolHash::operator()(
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

RiskResult RiskSupervisor::reject(
    RiskRejectReason reason,
    ouch::UserRefNum user_ref_num,
    ouch::Quantity quantity,
    ouch::Price4 price,
    Notional notional
) const noexcept {
    return RiskResult{
        .decision = RiskDecision::rejected,
        .reject_reason = reason,
        .user_ref_num = user_ref_num,
        .quantity = quantity,
        .price = price,
        .notional = notional
    };
}

RiskResult RiskSupervisor::accept(
    ouch::UserRefNum user_ref_num,
    ouch::Quantity quantity,
    ouch::Price4 price,
    Notional notional
) const noexcept {
    return RiskResult{
        .decision = RiskDecision::accepted,
        .reject_reason = RiskRejectReason::none,
        .user_ref_num = user_ref_num,
        .quantity = quantity,
        .price = price,
        .notional = notional
    };
}

RiskResult RiskSupervisor::check_common_order_risk(
    ouch::UserRefNum user_ref_num,
    ouch::Quantity quantity,
    ouch::Price4 price,
    const ouch::Symbol& symbol
) const noexcept {
    if (global_halted_) {
        return reject(
            RiskRejectReason::global_halt,
            user_ref_num,
            quantity,
            price
        );
    }

    if (cancel_only_) {
        return reject(
            RiskRejectReason::cancel_only,
            user_ref_num,
            quantity,
            price
        );
    }

    if (!is_symbol_enabled(symbol)) {
        return reject(
            RiskRejectReason::symbol_disabled,
            user_ref_num,
            quantity,
            price
        );
    }

    if (!ouch::is_valid_enter_or_replace_quantity(quantity)) {
        return reject(
            RiskRejectReason::invalid_quantity,
            user_ref_num,
            quantity,
            price
        );
    }

    if (!ouch::is_valid_price4(price)) {
        return reject(
            RiskRejectReason::invalid_price,
            user_ref_num,
            quantity,
            price
        );
    }

    if (quantity > limits_.max_order_quantity) {
        return reject(
            RiskRejectReason::max_order_quantity,
            user_ref_num,
            quantity,
            price
        );
    }

    if (exceeds_notional_limit(quantity, price)) {
        return reject(
            RiskRejectReason::max_order_notional,
            user_ref_num,
            quantity,
            price,
            calculate_order_notional(quantity, price)
        );
    }

    return accept(
        user_ref_num,
        quantity,
        price,
        calculate_order_notional(quantity, price)
    );
}

bool RiskSupervisor::exceeds_notional_limit(
    ouch::Quantity quantity,
    ouch::Price4 price
) const noexcept {
    if (quantity == 0 || price == 0) {
        return false;
    }

    if (price > std::numeric_limits<Notional>::max() / quantity) {
        return true;
    }

    return calculate_order_notional(quantity, price)
        > limits_.max_order_notional;
}

Notional RiskSupervisor::calculate_order_notional(
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

} // namespace fgep::risk