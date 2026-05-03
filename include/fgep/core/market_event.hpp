#pragma once

#include "fgep/core/time.hpp"
#include "fgep/core/types.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

namespace fgep {

// -----------------------------------------------------------------------------
// Nasdaq TotalView-ITCH 5.0 market event model
// -----------------------------------------------------------------------------
//
// This file models the ITCH messages that matter first for building a visible
// order book and tracking basic market state.
//
// The official Nasdaq TotalView-ITCH 5.0 specification defines the feed as a
// series of sequenced binary messages. The feed uses:
//   - big-endian integer fields
//   - ASCII alpha fields
//   - timestamps as nanoseconds since midnight
//   - stock locate codes as fast instrument identifiers
//   - fixed-point integer prices such as Price(4)
//
// This header does not parse raw binary packets yet.
// It defines strongly typed C++ structs that the future ITCH parser will emit.
//
// Later parser flow:
//
//   raw bytes
//   -> itch_parser
//   -> these ITCH message structs
//   -> visible book update / normalized market event
//   -> venue book
//   -> normalized BBO

// -----------------------------------------------------------------------------
// ITCH aliases
// -----------------------------------------------------------------------------

using StockLocate = std::uint16_t;
using TrackingNumber = std::uint16_t;
using MatchNumber = std::uint64_t;

// Nasdaq stock symbols in ITCH are 8-byte ASCII fields, right padded with spaces.
using StockSymbol = std::array<char, 8>;

// Nasdaq MPID attribution is a 4-byte ASCII field.
using Mpid = std::array<char, 4>;

// ITCH order reference numbers are day-unique identifiers assigned by Nasdaq.
using ItchOrderRef = std::uint64_t;

// Most visible order-book prices in TotalView-ITCH 5.0 are Price(4).
// That means the integer has four implied decimal places.
// Example:
//   1012500 means 101.2500
using ItchPrice4 = std::uint32_t;

// -----------------------------------------------------------------------------
// Common ITCH enums
// -----------------------------------------------------------------------------

enum class ItchMessageType : char {
    system_event = 'S',

    stock_directory = 'R',
    stock_trading_action = 'H',
    operational_halt = 'h',

    add_order_no_mpid = 'A',
    add_order_with_mpid = 'F',

    order_executed = 'E',
    order_executed_with_price = 'C',
    order_cancel = 'X',
    order_delete = 'D',
    order_replace = 'U',

    trade_non_cross = 'P',
    cross_trade = 'Q',
    broken_trade = 'B'
};

enum class ItchSide : char {
    buy = 'B',
    sell = 'S'
};

enum class SystemEventCode : char {
    start_of_messages = 'O',
    start_of_system_hours = 'S',
    start_of_market_hours = 'Q',
    end_of_market_hours = 'M',
    end_of_system_hours = 'E',
    end_of_messages = 'C'
};

enum class TradingState : char {
    halted = 'H',
    paused = 'P',
    quotation_only = 'Q',
    trading = 'T'
};

enum class OperationalHaltAction : char {
    halted = 'H',
    trading_resumed = 'T'
};

enum class PrintableFlag : char {
    non_printable = 'N',
    printable = 'Y'
};

enum class CrossType : char {
    opening_cross = 'O',
    closing_cross = 'C',
    ipo_or_halted = 'H'
};

// -----------------------------------------------------------------------------
// Shared message header
// -----------------------------------------------------------------------------
//
// In ITCH, most messages begin with:
//
//   Message Type      offset 0, length 1
//   Stock Locate      offset 1, length 2
//   Tracking Number   offset 3, length 2
//   Timestamp         offset 5, length 6
//
// The timestamp is nanoseconds since midnight.

struct ItchHeader {
    StockLocate stock_locate{};
    TrackingNumber tracking_number{};
    TimestampNs timestamp_ns{};
};

// -----------------------------------------------------------------------------
// Administrative messages
// -----------------------------------------------------------------------------

struct SystemEventMessage {
    ItchHeader header{};
    SystemEventCode event_code{SystemEventCode::start_of_messages};
};

struct StockDirectoryMessage {
    ItchHeader header{};
    StockSymbol stock{};
    char market_category{' '};
    char financial_status_indicator{' '};
    std::uint32_t round_lot_size{};
    char round_lots_only{' '};
    char issue_classification{' '};
    std::array<char, 2> issue_sub_type{};
    char authenticity{' '};
    char short_sale_threshold_indicator{' '};
    char ipo_flag{' '};
    char luld_reference_price_tier{' '};
    char etp_flag{' '};
    std::uint32_t etp_leverage_factor{};
    char inverse_indicator{' '};
};

struct StockTradingActionMessage {
    ItchHeader header{};
    StockSymbol stock{};
    TradingState trading_state{TradingState::halted};
    char reserved{' '};
    std::array<char, 4> reason{};
};

struct OperationalHaltMessage {
    ItchHeader header{};
    StockSymbol stock{};
    char market_code{' '};
    OperationalHaltAction halt_action{OperationalHaltAction::halted};
};

// -----------------------------------------------------------------------------
// Visible order-book messages
// -----------------------------------------------------------------------------

struct AddOrderNoMpidMessage {
    ItchHeader header{};
    ItchOrderRef order_reference_number{};
    ItchSide side{ItchSide::buy};
    Quantity shares{};
    StockSymbol stock{};
    ItchPrice4 price{};
};

struct AddOrderWithMpidMessage {
    ItchHeader header{};
    ItchOrderRef order_reference_number{};
    ItchSide side{ItchSide::buy};
    Quantity shares{};
    StockSymbol stock{};
    ItchPrice4 price{};
    Mpid attribution{};
};

struct OrderExecutedMessage {
    ItchHeader header{};
    ItchOrderRef order_reference_number{};
    Quantity executed_shares{};
    MatchNumber match_number{};
};

struct OrderExecutedWithPriceMessage {
    ItchHeader header{};
    ItchOrderRef order_reference_number{};
    Quantity executed_shares{};
    MatchNumber match_number{};
    PrintableFlag printable{PrintableFlag::printable};
    ItchPrice4 execution_price{};
};

struct OrderCancelMessage {
    ItchHeader header{};
    ItchOrderRef order_reference_number{};
    Quantity cancelled_shares{};
};

struct OrderDeleteMessage {
    ItchHeader header{};
    ItchOrderRef order_reference_number{};
};

struct OrderReplaceMessage {
    ItchHeader header{};
    ItchOrderRef original_order_reference_number{};
    ItchOrderRef new_order_reference_number{};
    Quantity shares{};
    ItchPrice4 price{};
};

// -----------------------------------------------------------------------------
// Trade messages
// -----------------------------------------------------------------------------

struct TradeNonCrossMessage {
    ItchHeader header{};
    ItchOrderRef order_reference_number{};
    ItchSide side{ItchSide::buy};
    Quantity shares{};
    StockSymbol stock{};
    ItchPrice4 price{};
    MatchNumber match_number{};
};

struct CrossTradeMessage {
    ItchHeader header{};
    Quantity shares{};
    StockSymbol stock{};
    ItchPrice4 cross_price{};
    MatchNumber match_number{};
    CrossType cross_type{CrossType::opening_cross};
};

struct BrokenTradeMessage {
    ItchHeader header{};
    MatchNumber match_number{};
};

// -----------------------------------------------------------------------------
// Variant over supported ITCH messages
// -----------------------------------------------------------------------------

using MarketEvent = std::variant<
    SystemEventMessage,
    StockDirectoryMessage,
    StockTradingActionMessage,
    OperationalHaltMessage,
    AddOrderNoMpidMessage,
    AddOrderWithMpidMessage,
    OrderExecutedMessage,
    OrderExecutedWithPriceMessage,
    OrderCancelMessage,
    OrderDeleteMessage,
    OrderReplaceMessage,
    TradeNonCrossMessage,
    CrossTradeMessage,
    BrokenTradeMessage
>;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr Side to_book_side(ItchSide side) noexcept {
    switch (side) {
        case ItchSide::buy:
            return Side::bid;
        case ItchSide::sell:
            return Side::ask;
    }

    return Side::bid;
}

[[nodiscard]] constexpr bool is_valid_stock_locate(
    StockLocate stock_locate
) noexcept {
    return stock_locate > 0;
}

[[nodiscard]] constexpr bool is_valid_itch_header(
    const ItchHeader& header
) noexcept {
    return is_valid_timestamp(header.timestamp_ns);
}

[[nodiscard]] constexpr bool is_valid_price4(ItchPrice4 price) noexcept {
    return price > 0;
}

[[nodiscard]] constexpr bool is_add_order(
    ItchMessageType message_type
) noexcept {
    return message_type == ItchMessageType::add_order_no_mpid
        || message_type == ItchMessageType::add_order_with_mpid;
}

[[nodiscard]] constexpr bool is_visible_book_update(
    ItchMessageType message_type
) noexcept {
    return message_type == ItchMessageType::add_order_no_mpid
        || message_type == ItchMessageType::add_order_with_mpid
        || message_type == ItchMessageType::order_executed
        || message_type == ItchMessageType::order_executed_with_price
        || message_type == ItchMessageType::order_cancel
        || message_type == ItchMessageType::order_delete
        || message_type == ItchMessageType::order_replace;
}

[[nodiscard]] constexpr bool affects_visible_book(
    const AddOrderNoMpidMessage& message
) noexcept {
    return is_valid_itch_header(message.header)
        && message.order_reference_number > 0
        && is_valid_quantity(message.shares)
        && is_valid_price4(message.price);
}

[[nodiscard]] constexpr bool affects_visible_book(
    const AddOrderWithMpidMessage& message
) noexcept {
    return is_valid_itch_header(message.header)
        && message.order_reference_number > 0
        && is_valid_quantity(message.shares)
        && is_valid_price4(message.price);
}

[[nodiscard]] constexpr bool affects_visible_book(
    const OrderExecutedMessage& message
) noexcept {
    return is_valid_itch_header(message.header)
        && message.order_reference_number > 0
        && is_valid_quantity(message.executed_shares);
}

[[nodiscard]] constexpr bool affects_visible_book(
    const OrderExecutedWithPriceMessage& message
) noexcept {
    return is_valid_itch_header(message.header)
        && message.order_reference_number > 0
        && is_valid_quantity(message.executed_shares)
        && is_valid_price4(message.execution_price);
}

[[nodiscard]] constexpr bool affects_visible_book(
    const OrderCancelMessage& message
) noexcept {
    return is_valid_itch_header(message.header)
        && message.order_reference_number > 0
        && is_valid_quantity(message.cancelled_shares);
}

[[nodiscard]] constexpr bool affects_visible_book(
    const OrderDeleteMessage& message
) noexcept {
    return is_valid_itch_header(message.header)
        && message.order_reference_number > 0;
}

[[nodiscard]] constexpr bool affects_visible_book(
    const OrderReplaceMessage& message
) noexcept {
    return is_valid_itch_header(message.header)
        && message.original_order_reference_number > 0
        && message.new_order_reference_number > 0
        && is_valid_quantity(message.shares)
        && is_valid_price4(message.price);
}

[[nodiscard]] constexpr bool affects_visible_book(
    const TradeNonCrossMessage&
) noexcept {
    return false;
}

[[nodiscard]] constexpr bool affects_visible_book(
    const CrossTradeMessage&
) noexcept {
    return false;
}

[[nodiscard]] constexpr bool affects_visible_book(
    const BrokenTradeMessage&
) noexcept {
    return false;
}

[[nodiscard]] constexpr std::string_view to_string(
    ItchMessageType message_type
) noexcept {
    switch (message_type) {
        case ItchMessageType::system_event:
            return "system_event";
        case ItchMessageType::stock_directory:
            return "stock_directory";
        case ItchMessageType::stock_trading_action:
            return "stock_trading_action";
        case ItchMessageType::operational_halt:
            return "operational_halt";
        case ItchMessageType::add_order_no_mpid:
            return "add_order_no_mpid";
        case ItchMessageType::add_order_with_mpid:
            return "add_order_with_mpid";
        case ItchMessageType::order_executed:
            return "order_executed";
        case ItchMessageType::order_executed_with_price:
            return "order_executed_with_price";
        case ItchMessageType::order_cancel:
            return "order_cancel";
        case ItchMessageType::order_delete:
            return "order_delete";
        case ItchMessageType::order_replace:
            return "order_replace";
        case ItchMessageType::trade_non_cross:
            return "trade_non_cross";
        case ItchMessageType::cross_trade:
            return "cross_trade";
        case ItchMessageType::broken_trade:
            return "broken_trade";
    }

    return "unknown";
}

} // namespace fgep