#pragma once

#include <cstdint>
#include <string_view>

namespace fgep {

// -----------------------------------------------------------------------------
// numeric domain types
// -----------------------------------------------------------------------------
//
// These aliases define the shared vocabulary used across replay, book building,
// risk checks, execution simulation, telemetry, and hardware-gate modeling.
//
// Prices are represented as integer ticks, not floating-point values.
// Example:
//   If the tick scale is 1 / 10,000 dollars, then $101.2345 is stored as 1012345.
//
// Quantities are represented as integer units.
// Notional is represented as integer price_ticks * quantity.


using SymbolId = std::uint32_t;
using VenueId = std::uint16_t;

using OrderId = std::uint64_t;
using ClientOrderId = std::uint64_t;

using Price = std::int64_t;
using Quantity = std::uint64_t;
using Notional = std::uint64_t;

using SequenceNumber = std::uint64_t;
using TimestampNs = std::uint64_t;

// -----------------------------------------------------------------------------
// enums
// -----------------------------------------------------------------------------

enum class Side : std::uint8_t {
    bid,
    ask
};

enum class OrderAction : std::uint8_t {
    new_order,
    cancel,
    replace
};

enum class RejectReason : std::uint8_t {
    none,
    unknown_symbol,
    unknown_order,
    duplicate_order_id,
    invalid_quantity,
    invalid_price,
    risk_rejected,
    invalid_state_transition
};

// -----------------------------------------------------------------------------
// helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr bool is_bid(Side side) noexcept {
    return side == Side::bid;
}

[[nodiscard]] constexpr bool is_ask(Side side) noexcept {
    return side == Side::ask;
}

[[nodiscard]] constexpr std::string_view to_string(Side side) noexcept {
    switch (side) {
        case Side::bid:
            return "bid";
        case Side::ask:
            return "ask";
    }

    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(OrderAction action) noexcept {
    switch (action) {
        case OrderAction::new_order:
            return "new_order";
        case OrderAction::cancel:
            return "cancel";
        case OrderAction::replace:
            return "replace";
    }

    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(RejectReason reason) noexcept {
    switch (reason) {
        case RejectReason::none:
            return "none";
        case RejectReason::unknown_symbol:
            return "unknown_symbol";
        case RejectReason::unknown_order:
            return "unknown_order";
        case RejectReason::duplicate_order_id:
            return "duplicate_order_id";
        case RejectReason::invalid_quantity:
            return "invalid_quantity";
        case RejectReason::invalid_price:
            return "invalid_price";
        case RejectReason::risk_rejected:
            return "risk_rejected";
        case RejectReason::invalid_state_transition:
            return "invalid_state_transition";
    }

    return "unknown";
}

[[nodiscard]] constexpr bool is_valid_price(Price price) noexcept {
    return price > 0;
}

[[nodiscard]] constexpr bool is_valid_quantity(Quantity quantity) noexcept {
    return quantity > 0;
}

[[nodiscard]] constexpr Notional calculate_notional(
    Price price,
    Quantity quantity
) noexcept {
    if (price <= 0) {
        return 0;
    }

    return static_cast<Notional>(price) * quantity;
}

} // namespace fgep