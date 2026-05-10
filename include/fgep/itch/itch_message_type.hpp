#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/itch/itch_types.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace fgep::itch {

// -----------------------------------------------------------------------------
// Nasdaq TotalView-ITCH 5.0 message type model
// -----------------------------------------------------------------------------
//
// This file defines the one-byte message type values used by Nasdaq
// TotalView-ITCH 5.0.
// -----------------------------------------------------------------------------
// Message type enum
// -----------------------------------------------------------------------------

enum class MessageType : char {
    // 1.1 System Event Message
    system_event = 'S',

    // 1.2 Stock Related Messages
    stock_directory = 'R',
    stock_trading_action = 'H',
    reg_sho_restriction = 'Y',
    market_participant_position = 'L',
    mwcb_decline_level = 'V',
    mwcb_status = 'W',
    ipo_quoting_period_update = 'K',
    luld_auction_collar = 'J',
    operational_halt = 'h',

    // 1.3 Add Order Messages
    add_order_no_mpid = 'A',
    add_order_with_mpid = 'F',

    // 1.4 Modify Order Messages
    order_executed = 'E',
    order_executed_with_price = 'C',
    order_cancel = 'X',
    order_delete = 'D',
    order_replace = 'U',

    // 1.5 Trade Messages
    trade_non_cross = 'P',
    cross_trade = 'Q',
    broken_trade = 'B',

    // 1.6 Net Order Imbalance Indicator
    noii = 'I',

    // 1.7 Retail Price Improvement Indicator
    retail_price_improvement_indicator = 'N',

    // 1.8 Direct Listing with Capital Raise Price Discovery Message
    direct_listing_with_capital_raise = 'O'
};

// -----------------------------------------------------------------------------
// Conversion helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr char to_char(MessageType message_type) noexcept {
    return static_cast<char>(message_type);
}

[[nodiscard]] constexpr Result<MessageType> message_type_from_char(
    char value
) noexcept {
    switch (value) {
        case 'S':
            return {ErrorCode::ok, MessageType::system_event};

        case 'R':
            return {ErrorCode::ok, MessageType::stock_directory};
        case 'H':
            return {ErrorCode::ok, MessageType::stock_trading_action};
        case 'Y':
            return {ErrorCode::ok, MessageType::reg_sho_restriction};
        case 'L':
            return {ErrorCode::ok, MessageType::market_participant_position};
        case 'V':
            return {ErrorCode::ok, MessageType::mwcb_decline_level};
        case 'W':
            return {ErrorCode::ok, MessageType::mwcb_status};
        case 'K':
            return {ErrorCode::ok, MessageType::ipo_quoting_period_update};
        case 'J':
            return {ErrorCode::ok, MessageType::luld_auction_collar};
        case 'h':
            return {ErrorCode::ok, MessageType::operational_halt};

        case 'A':
            return {ErrorCode::ok, MessageType::add_order_no_mpid};
        case 'F':
            return {ErrorCode::ok, MessageType::add_order_with_mpid};

        case 'E':
            return {ErrorCode::ok, MessageType::order_executed};
        case 'C':
            return {ErrorCode::ok, MessageType::order_executed_with_price};
        case 'X':
            return {ErrorCode::ok, MessageType::order_cancel};
        case 'D':
            return {ErrorCode::ok, MessageType::order_delete};
        case 'U':
            return {ErrorCode::ok, MessageType::order_replace};

        case 'P':
            return {ErrorCode::ok, MessageType::trade_non_cross};
        case 'Q':
            return {ErrorCode::ok, MessageType::cross_trade};
        case 'B':
            return {ErrorCode::ok, MessageType::broken_trade};

        case 'I':
            return {ErrorCode::ok, MessageType::noii};
        case 'N':
            return {
                ErrorCode::ok,
                MessageType::retail_price_improvement_indicator
            };
        case 'O':
            return {
                ErrorCode::ok,
                MessageType::direct_listing_with_capital_raise
            };

        default:
            return {ErrorCode::parse_error, MessageType::system_event};
    }
}

[[nodiscard]] constexpr bool is_known_message_type(char value) noexcept {
    return message_type_from_char(value).ok();
}

// -----------------------------------------------------------------------------
// Human-readable names
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr std::string_view to_string(
    MessageType message_type
) noexcept {
    switch (message_type) {
        case MessageType::system_event:
            return "system_event";

        case MessageType::stock_directory:
            return "stock_directory";
        case MessageType::stock_trading_action:
            return "stock_trading_action";
        case MessageType::reg_sho_restriction:
            return "reg_sho_restriction";
        case MessageType::market_participant_position:
            return "market_participant_position";
        case MessageType::mwcb_decline_level:
            return "mwcb_decline_level";
        case MessageType::mwcb_status:
            return "mwcb_status";
        case MessageType::ipo_quoting_period_update:
            return "ipo_quoting_period_update";
        case MessageType::luld_auction_collar:
            return "luld_auction_collar";
        case MessageType::operational_halt:
            return "operational_halt";

        case MessageType::add_order_no_mpid:
            return "add_order_no_mpid";
        case MessageType::add_order_with_mpid:
            return "add_order_with_mpid";

        case MessageType::order_executed:
            return "order_executed";
        case MessageType::order_executed_with_price:
            return "order_executed_with_price";
        case MessageType::order_cancel:
            return "order_cancel";
        case MessageType::order_delete:
            return "order_delete";
        case MessageType::order_replace:
            return "order_replace";

        case MessageType::trade_non_cross:
            return "trade_non_cross";
        case MessageType::cross_trade:
            return "cross_trade";
        case MessageType::broken_trade:
            return "broken_trade";

        case MessageType::noii:
            return "noii";
        case MessageType::retail_price_improvement_indicator:
            return "retail_price_improvement_indicator";
        case MessageType::direct_listing_with_capital_raise:
            return "direct_listing_with_capital_raise";

    }

    return "unknown";
}

// -----------------------------------------------------------------------------
// Message category helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr bool is_administrative_message(
    MessageType message_type
) noexcept {
    switch (message_type) {
        case MessageType::system_event:
        case MessageType::stock_directory:
        case MessageType::stock_trading_action:
        case MessageType::reg_sho_restriction:
        case MessageType::market_participant_position:
        case MessageType::mwcb_decline_level:
        case MessageType::mwcb_status:
        case MessageType::ipo_quoting_period_update:
        case MessageType::luld_auction_collar:
        case MessageType::operational_halt:
        case MessageType::noii:
        case MessageType::retail_price_improvement_indicator:
        case MessageType::direct_listing_with_capital_raise:
            return true;

        case MessageType::add_order_no_mpid:
        case MessageType::add_order_with_mpid:
        case MessageType::order_executed:
        case MessageType::order_executed_with_price:
        case MessageType::order_cancel:
        case MessageType::order_delete:
        case MessageType::order_replace:
        case MessageType::trade_non_cross:
        case MessageType::cross_trade:
        case MessageType::broken_trade:
            return false;
    }

    return false;
}

[[nodiscard]] constexpr bool is_add_order_message(
    MessageType message_type
) noexcept {
    return message_type == MessageType::add_order_no_mpid
        || message_type == MessageType::add_order_with_mpid;
}

[[nodiscard]] constexpr bool is_order_book_modify_message(
    MessageType message_type
) noexcept {
    return message_type == MessageType::order_executed
        || message_type == MessageType::order_executed_with_price
        || message_type == MessageType::order_cancel
        || message_type == MessageType::order_delete
        || message_type == MessageType::order_replace;
}

[[nodiscard]] constexpr bool is_visible_book_update_message(
    MessageType message_type
) noexcept {
    return is_add_order_message(message_type)
        || is_order_book_modify_message(message_type);
}

[[nodiscard]] constexpr bool is_trade_message(
    MessageType message_type
) noexcept {
    return message_type == MessageType::trade_non_cross
        || message_type == MessageType::cross_trade
        || message_type == MessageType::broken_trade;
}

// -----------------------------------------------------------------------------
// Exact message lengths
// -----------------------------------------------------------------------------
//
// These are byte lengths for the TotalView-ITCH payload message itself.
// They do not include any SoupBinTCP, MoldUDP64, or other transport framing.
//
// Some messages are intentionally left as unsupported if they are not in the
// first byte-to-byte implementation set.

[[nodiscard]] constexpr Result<std::size_t> message_length(
    MessageType message_type
) noexcept {
    switch (message_type) {
        case MessageType::system_event:
            return {ErrorCode::ok, length_system_event};

        case MessageType::add_order_no_mpid:
            return {ErrorCode::ok, length_add_order_no_mpid};
        case MessageType::add_order_with_mpid:
            return {ErrorCode::ok, length_add_order_with_mpid};

        case MessageType::order_executed:
            return {ErrorCode::ok, length_order_executed};
        case MessageType::order_executed_with_price:
            return {ErrorCode::ok, length_order_executed_with_price};
        case MessageType::order_cancel:
            return {ErrorCode::ok, length_order_cancel};
        case MessageType::order_delete:
            return {ErrorCode::ok, length_order_delete};
        case MessageType::order_replace:
            return {ErrorCode::ok, length_order_replace};

        case MessageType::trade_non_cross:
            return {ErrorCode::ok, length_trade_non_cross};
        case MessageType::cross_trade:
            return {ErrorCode::ok, length_cross_trade};
        case MessageType::broken_trade:
            return {ErrorCode::ok, length_broken_trade};
        case MessageType::reg_sho_restriction:
        case MessageType::market_participant_position:
        case MessageType::mwcb_decline_level:
        case MessageType::mwcb_status:
        case MessageType::ipo_quoting_period_update:
        case MessageType::luld_auction_collar:
        case MessageType::operational_halt:
        case MessageType::noii:
        case MessageType::retail_price_improvement_indicator:
        case MessageType::direct_listing_with_capital_raise:
            return {ErrorCode::unsupported, 0};
        case MessageType::stock_directory:
            return {ErrorCode::ok, length_stock_directory};
        case MessageType::stock_trading_action:
            return {ErrorCode::ok, length_stock_trading_action};
    }

    return {ErrorCode::unsupported, 0};
}

[[nodiscard]] constexpr bool has_supported_length(
    MessageType message_type
) noexcept {
    return message_length(message_type).ok();
}

} // namespace fgep::itch