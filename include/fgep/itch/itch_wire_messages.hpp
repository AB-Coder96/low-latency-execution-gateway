#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/itch/itch_message_type.hpp"
#include "fgep/itch/itch_types.hpp"

#include <cstddef>
#include <string_view>
#include <variant>

namespace fgep::itch {

// -----------------------------------------------------------------------------
// Nasdaq TotalView-ITCH 5.0 decoded wire messages
// -----------------------------------------------------------------------------
//
// This file defines decoded C++ representations of exact ITCH wire messages.
//
// Important:
//   These are NOT meant to be reinterpret_cast from raw bytes.
//
//   S = System Event
//   A = Add Order - No MPID Attribution
//   F = Add Order with MPID Attribution
//   E = Order Executed
//   C = Order Executed with Price
//   X = Order Cancel
//   D = Order Delete
//   U = Order Replace
//   P = Trade Non-Cross
//   Q = Cross Trade
//   B = Broken Trade

// -----------------------------------------------------------------------------
// Protocol field enums
// -----------------------------------------------------------------------------

enum class SystemEventCode : char {
    start_of_messages = 'O',
    start_of_system_hours = 'S',
    start_of_market_hours = 'Q',
    end_of_market_hours = 'M',
    end_of_system_hours = 'E',
    end_of_messages = 'C'
};

enum class Side : char {
    buy = 'B',
    sell = 'S'
};

enum class Printable : char {
    non_printable = 'N',
    printable = 'Y'
};

enum class CrossType : char {
    opening_cross = 'O',
    closing_cross = 'C',
    ipo_or_halted = 'H'
};

// -----------------------------------------------------------------------------
// Exact field offsets
// -----------------------------------------------------------------------------
//
// Common ITCH header offsets are defined in itch_types.hpp:
//
//   offset_message_type      = 0
//   offset_stock_locate      = 1
//   offset_tracking_number   = 3
//   offset_timestamp         = 5
//   offset_message_body      = 11
//

// S - System Event Message
inline constexpr std::size_t system_event_offset_event_code = 11;

// A - Add Order, No MPID Attribution
inline constexpr std::size_t add_order_offset_order_reference_number = 11;
inline constexpr std::size_t add_order_offset_side = 19;
inline constexpr std::size_t add_order_offset_shares = 20;
inline constexpr std::size_t add_order_offset_stock = 24;
inline constexpr std::size_t add_order_offset_price = 32;

// F - Add Order with MPID Attribution
inline constexpr std::size_t add_order_with_mpid_offset_order_reference_number =
    add_order_offset_order_reference_number;
inline constexpr std::size_t add_order_with_mpid_offset_side =
    add_order_offset_side;
inline constexpr std::size_t add_order_with_mpid_offset_shares =
    add_order_offset_shares;
inline constexpr std::size_t add_order_with_mpid_offset_stock =
    add_order_offset_stock;
inline constexpr std::size_t add_order_with_mpid_offset_price =
    add_order_offset_price;
inline constexpr std::size_t add_order_with_mpid_offset_attribution = 36;

// E - Order Executed
inline constexpr std::size_t order_executed_offset_order_reference_number = 11;
inline constexpr std::size_t order_executed_offset_executed_shares = 19;
inline constexpr std::size_t order_executed_offset_match_number = 23;

// C - Order Executed with Price
inline constexpr std::size_t order_executed_with_price_offset_order_reference_number =
    11;
inline constexpr std::size_t order_executed_with_price_offset_executed_shares =
    19;
inline constexpr std::size_t order_executed_with_price_offset_match_number = 23;
inline constexpr std::size_t order_executed_with_price_offset_printable = 31;
inline constexpr std::size_t order_executed_with_price_offset_execution_price =
    32;

// X - Order Cancel
inline constexpr std::size_t order_cancel_offset_order_reference_number = 11;
inline constexpr std::size_t order_cancel_offset_cancelled_shares = 19;

// D - Order Delete
inline constexpr std::size_t order_delete_offset_order_reference_number = 11;

// U - Order Replace
inline constexpr std::size_t order_replace_offset_original_order_reference_number =
    11;
inline constexpr std::size_t order_replace_offset_new_order_reference_number =
    19;
inline constexpr std::size_t order_replace_offset_shares = 27;
inline constexpr std::size_t order_replace_offset_price = 31;

// P - Trade Message, Non-Cross
inline constexpr std::size_t trade_non_cross_offset_order_reference_number = 11;
inline constexpr std::size_t trade_non_cross_offset_side = 19;
inline constexpr std::size_t trade_non_cross_offset_shares = 20;
inline constexpr std::size_t trade_non_cross_offset_stock = 24;
inline constexpr std::size_t trade_non_cross_offset_price = 32;
inline constexpr std::size_t trade_non_cross_offset_match_number = 36;

// Q - Cross Trade
inline constexpr std::size_t cross_trade_offset_shares = 11;
inline constexpr std::size_t cross_trade_offset_stock = 19;
inline constexpr std::size_t cross_trade_offset_cross_price = 27;
inline constexpr std::size_t cross_trade_offset_match_number = 31;
inline constexpr std::size_t cross_trade_offset_cross_type = 39;

// B - Broken Trade
inline constexpr std::size_t broken_trade_offset_match_number = 11;

// -----------------------------------------------------------------------------
// Decoded wire-message structs
// -----------------------------------------------------------------------------

struct SystemEventMessage {
    Header header{};
    SystemEventCode event_code{SystemEventCode::start_of_messages};
};

struct AddOrderNoMpidMessage {
    Header header{};
    OrderReferenceNumber order_reference_number{};
    Side side{Side::buy};
    Shares shares{};
    StockSymbol stock{};
    Price4 price{};
};

struct AddOrderWithMpidMessage {
    Header header{};
    OrderReferenceNumber order_reference_number{};
    Side side{Side::buy};
    Shares shares{};
    StockSymbol stock{};
    Price4 price{};
    Mpid attribution{};
};

struct OrderExecutedMessage {
    Header header{};
    OrderReferenceNumber order_reference_number{};
    Shares executed_shares{};
    MatchNumber match_number{};
};

struct OrderExecutedWithPriceMessage {
    Header header{};
    OrderReferenceNumber order_reference_number{};
    Shares executed_shares{};
    MatchNumber match_number{};
    Printable printable{Printable::printable};
    Price4 execution_price{};
};

struct OrderCancelMessage {
    Header header{};
    OrderReferenceNumber order_reference_number{};
    Shares cancelled_shares{};
};

struct OrderDeleteMessage {
    Header header{};
    OrderReferenceNumber order_reference_number{};
};

struct OrderReplaceMessage {
    Header header{};
    OrderReferenceNumber original_order_reference_number{};
    OrderReferenceNumber new_order_reference_number{};
    Shares shares{};
    Price4 price{};
};

struct TradeNonCrossMessage {
    Header header{};
    OrderReferenceNumber order_reference_number{};
    Side side{Side::buy};
    Shares shares{};
    StockSymbol stock{};
    Price4 price{};
    MatchNumber match_number{};
};

struct CrossTradeMessage {
    Header header{};
    Shares shares{};
    StockSymbol stock{};
    Price4 cross_price{};
    MatchNumber match_number{};
    CrossType cross_type{CrossType::opening_cross};
};

struct BrokenTradeMessage {
    Header header{};
    MatchNumber match_number{};
};

using Message = std::variant<
    SystemEventMessage,
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
// Field enum conversions
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr char to_char(SystemEventCode value) noexcept {
    return static_cast<char>(value);
}

[[nodiscard]] constexpr char to_char(Side value) noexcept {
    return static_cast<char>(value);
}

[[nodiscard]] constexpr char to_char(Printable value) noexcept {
    return static_cast<char>(value);
}

[[nodiscard]] constexpr char to_char(CrossType value) noexcept {
    return static_cast<char>(value);
}

[[nodiscard]] constexpr Result<SystemEventCode> system_event_code_from_char(
    char value
) noexcept {
    switch (value) {
        case 'O':
            return {ErrorCode::ok, SystemEventCode::start_of_messages};
        case 'S':
            return {ErrorCode::ok, SystemEventCode::start_of_system_hours};
        case 'Q':
            return {ErrorCode::ok, SystemEventCode::start_of_market_hours};
        case 'M':
            return {ErrorCode::ok, SystemEventCode::end_of_market_hours};
        case 'E':
            return {ErrorCode::ok, SystemEventCode::end_of_system_hours};
        case 'C':
            return {ErrorCode::ok, SystemEventCode::end_of_messages};
        default:
            return {
                ErrorCode::parse_error,
                SystemEventCode::start_of_messages
            };
    }
}

[[nodiscard]] constexpr Result<Side> side_from_char(char value) noexcept {
    switch (value) {
        case 'B':
            return {ErrorCode::ok, Side::buy};
        case 'S':
            return {ErrorCode::ok, Side::sell};
        default:
            return {ErrorCode::parse_error, Side::buy};
    }
}

[[nodiscard]] constexpr Result<Printable> printable_from_char(
    char value
) noexcept {
    switch (value) {
        case 'N':
            return {ErrorCode::ok, Printable::non_printable};
        case 'Y':
            return {ErrorCode::ok, Printable::printable};
        default:
            return {ErrorCode::parse_error, Printable::printable};
    }
}

[[nodiscard]] constexpr Result<CrossType> cross_type_from_char(
    char value
) noexcept {
    switch (value) {
        case 'O':
            return {ErrorCode::ok, CrossType::opening_cross};
        case 'C':
            return {ErrorCode::ok, CrossType::closing_cross};
        case 'H':
            return {ErrorCode::ok, CrossType::ipo_or_halted};
        default:
            return {ErrorCode::parse_error, CrossType::opening_cross};
    }
}

// -----------------------------------------------------------------------------
// Field enum string helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr std::string_view to_string(
    SystemEventCode value
) noexcept {
    switch (value) {
        case SystemEventCode::start_of_messages:
            return "start_of_messages";
        case SystemEventCode::start_of_system_hours:
            return "start_of_system_hours";
        case SystemEventCode::start_of_market_hours:
            return "start_of_market_hours";
        case SystemEventCode::end_of_market_hours:
            return "end_of_market_hours";
        case SystemEventCode::end_of_system_hours:
            return "end_of_system_hours";
        case SystemEventCode::end_of_messages:
            return "end_of_messages";
    }

    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(Side value) noexcept {
    switch (value) {
        case Side::buy:
            return "buy";
        case Side::sell:
            return "sell";
    }

    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(Printable value) noexcept {
    switch (value) {
        case Printable::non_printable:
            return "non_printable";
        case Printable::printable:
            return "printable";
    }

    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(CrossType value) noexcept {
    switch (value) {
        case CrossType::opening_cross:
            return "opening_cross";
        case CrossType::closing_cross:
            return "closing_cross";
        case CrossType::ipo_or_halted:
            return "ipo_or_halted";
    }

    return "unknown";
}

// -----------------------------------------------------------------------------
// Message metadata helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr MessageType message_type(
    const SystemEventMessage&
) noexcept {
    return MessageType::system_event;
}

[[nodiscard]] constexpr MessageType message_type(
    const AddOrderNoMpidMessage&
) noexcept {
    return MessageType::add_order_no_mpid;
}

[[nodiscard]] constexpr MessageType message_type(
    const AddOrderWithMpidMessage&
) noexcept {
    return MessageType::add_order_with_mpid;
}

[[nodiscard]] constexpr MessageType message_type(
    const OrderExecutedMessage&
) noexcept {
    return MessageType::order_executed;
}

[[nodiscard]] constexpr MessageType message_type(
    const OrderExecutedWithPriceMessage&
) noexcept {
    return MessageType::order_executed_with_price;
}

[[nodiscard]] constexpr MessageType message_type(
    const OrderCancelMessage&
) noexcept {
    return MessageType::order_cancel;
}

[[nodiscard]] constexpr MessageType message_type(
    const OrderDeleteMessage&
) noexcept {
    return MessageType::order_delete;
}

[[nodiscard]] constexpr MessageType message_type(
    const OrderReplaceMessage&
) noexcept {
    return MessageType::order_replace;
}

[[nodiscard]] constexpr MessageType message_type(
    const TradeNonCrossMessage&
) noexcept {
    return MessageType::trade_non_cross;
}

[[nodiscard]] constexpr MessageType message_type(
    const CrossTradeMessage&
) noexcept {
    return MessageType::cross_trade;
}

[[nodiscard]] constexpr MessageType message_type(
    const BrokenTradeMessage&
) noexcept {
    return MessageType::broken_trade;
}

[[nodiscard]] constexpr std::size_t wire_length(
    const SystemEventMessage&
) noexcept {
    return length_system_event;
}

[[nodiscard]] constexpr std::size_t wire_length(
    const AddOrderNoMpidMessage&
) noexcept {
    return length_add_order_no_mpid;
}

[[nodiscard]] constexpr std::size_t wire_length(
    const AddOrderWithMpidMessage&
) noexcept {
    return length_add_order_with_mpid;
}

[[nodiscard]] constexpr std::size_t wire_length(
    const OrderExecutedMessage&
) noexcept {
    return length_order_executed;
}

[[nodiscard]] constexpr std::size_t wire_length(
    const OrderExecutedWithPriceMessage&
) noexcept {
    return length_order_executed_with_price;
}

[[nodiscard]] constexpr std::size_t wire_length(
    const OrderCancelMessage&
) noexcept {
    return length_order_cancel;
}

[[nodiscard]] constexpr std::size_t wire_length(
    const OrderDeleteMessage&
) noexcept {
    return length_order_delete;
}

[[nodiscard]] constexpr std::size_t wire_length(
    const OrderReplaceMessage&
) noexcept {
    return length_order_replace;
}

[[nodiscard]] constexpr std::size_t wire_length(
    const TradeNonCrossMessage&
) noexcept {
    return length_trade_non_cross;
}

[[nodiscard]] constexpr std::size_t wire_length(
    const CrossTradeMessage&
) noexcept {
    return length_cross_trade;
}

[[nodiscard]] constexpr std::size_t wire_length(
    const BrokenTradeMessage&
) noexcept {
    return length_broken_trade;
}

// -----------------------------------------------------------------------------
// Validation helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr bool is_valid_system_event_message(
    const SystemEventMessage& message
) noexcept {
    return is_valid_header_allowing_zero_stock_locate(message.header);
}

[[nodiscard]] constexpr bool is_valid_add_order_no_mpid_message(
    const AddOrderNoMpidMessage& message
) noexcept {
    return is_valid_header_for_stock_message(message.header)
        && is_valid_order_reference_number(message.order_reference_number)
        && is_valid_shares(message.shares)
        && is_valid_price4(message.price)
        && wire::is_valid_fixed_ascii(message.stock);
}

[[nodiscard]] constexpr bool is_valid_add_order_with_mpid_message(
    const AddOrderWithMpidMessage& message
) noexcept {
    return is_valid_header_for_stock_message(message.header)
        && is_valid_order_reference_number(message.order_reference_number)
        && is_valid_shares(message.shares)
        && is_valid_price4(message.price)
        && wire::is_valid_fixed_ascii(message.stock)
        && wire::is_valid_fixed_ascii(message.attribution);
}

[[nodiscard]] constexpr bool is_valid_order_executed_message(
    const OrderExecutedMessage& message
) noexcept {
    return is_valid_header_for_stock_message(message.header)
        && is_valid_order_reference_number(message.order_reference_number)
        && is_valid_shares(message.executed_shares)
        && is_valid_match_number(message.match_number);
}

[[nodiscard]] constexpr bool is_valid_order_executed_with_price_message(
    const OrderExecutedWithPriceMessage& message
) noexcept {
    return is_valid_header_for_stock_message(message.header)
        && is_valid_order_reference_number(message.order_reference_number)
        && is_valid_shares(message.executed_shares)
        && is_valid_match_number(message.match_number)
        && is_valid_price4(message.execution_price);
}

[[nodiscard]] constexpr bool is_valid_order_cancel_message(
    const OrderCancelMessage& message
) noexcept {
    return is_valid_header_for_stock_message(message.header)
        && is_valid_order_reference_number(message.order_reference_number)
        && is_valid_shares(message.cancelled_shares);
}

[[nodiscard]] constexpr bool is_valid_order_delete_message(
    const OrderDeleteMessage& message
) noexcept {
    return is_valid_header_for_stock_message(message.header)
        && is_valid_order_reference_number(message.order_reference_number);
}

[[nodiscard]] constexpr bool is_valid_order_replace_message(
    const OrderReplaceMessage& message
) noexcept {
    return is_valid_header_for_stock_message(message.header)
        && is_valid_order_reference_number(
            message.original_order_reference_number
        )
        && is_valid_order_reference_number(message.new_order_reference_number)
        && message.original_order_reference_number
            != message.new_order_reference_number
        && is_valid_shares(message.shares)
        && is_valid_price4(message.price);
}

[[nodiscard]] constexpr bool is_valid_trade_non_cross_message(
    const TradeNonCrossMessage& message
) noexcept {
    return is_valid_header_for_stock_message(message.header)
        && is_valid_shares(message.shares)
        && is_valid_price4(message.price)
        && is_valid_match_number(message.match_number)
        && wire::is_valid_fixed_ascii(message.stock);
}

[[nodiscard]] constexpr bool is_valid_cross_trade_message(
    const CrossTradeMessage& message
) noexcept {
    return is_valid_header_for_stock_message(message.header)
        && is_valid_shares(message.shares)
        && is_valid_price4(message.cross_price)
        && is_valid_match_number(message.match_number)
        && wire::is_valid_fixed_ascii(message.stock);
}

[[nodiscard]] constexpr bool is_valid_broken_trade_message(
    const BrokenTradeMessage& message
) noexcept {
    return is_valid_header_for_stock_message(message.header)
        && is_valid_match_number(message.match_number);
}

} // namespace fgep::itch