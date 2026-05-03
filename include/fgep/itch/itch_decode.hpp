#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/itch/itch_message_type.hpp"
#include "fgep/itch/itch_wire_messages.hpp"

#include <cstddef>
#include <span>

namespace fgep::itch {

// -----------------------------------------------------------------------------
// Nasdaq TotalView-ITCH 5.0 byte decoder API
// -----------------------------------------------------------------------------
//
// This header declares the byte-to-message decode functions for ITCH payload
// messages.
//
// This is the byte-to-byte protocol layer:
//
//   raw ITCH payload bytes
//   -> explicit offset reads
//   -> decoded itch::*Message struct
//
// Important:
//   These functions decode one ITCH payload message at a time.
//   They do not decode SoupBinTCP framing.
//   They do not decode MoldUDP64 packet framing.
//   They do not reinterpret bytes as packed structs.
//
// Transport framing should be handled by a separate module later:
//
//   soupbintcp/
//   moldudp64/
//
// This decoder expects the input span to point at the start of the ITCH payload:
//
//   bytes[0] = ITCH Message Type
//
// Common ITCH header layout:
//
//   Offset  Length  Field
//   0       1       Message Type
//   1       2       Stock Locate
//   3       2       Tracking Number
//   5       6       Timestamp, nanoseconds since midnight
//
// The message-specific body begins at offset 11.

// -----------------------------------------------------------------------------
// Generic decode helpers
// -----------------------------------------------------------------------------

[[nodiscard]] Result<MessageType> decode_message_type(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] Result<Header> decode_header(
    std::span<const std::byte> bytes
) noexcept;

// Checks that:
//   1. bytes contains at least one byte
//   2. bytes[0] is the expected message type
//   3. bytes.size() equals the exact expected message length
//
// This is stricter than "at least length" on purpose.
// Byte-to-byte tests should fail if an unexpected trailing byte is present.
[[nodiscard]] ErrorCode require_exact_message_type_and_length(
    std::span<const std::byte> bytes,
    MessageType expected_message_type
) noexcept;

// Dispatches by bytes[0] and decodes one supported ITCH message.
//
// Unsupported known message types return ErrorCode::unsupported.
// Unknown message types return ErrorCode::parse_error.
[[nodiscard]] Result<Message> decode_message(
    std::span<const std::byte> bytes
) noexcept;

// -----------------------------------------------------------------------------
// Message-specific decoders
// -----------------------------------------------------------------------------
//
// First implementation set:
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

[[nodiscard]] Result<SystemEventMessage> decode_system_event_message(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] Result<AddOrderNoMpidMessage> decode_add_order_no_mpid_message(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] Result<AddOrderWithMpidMessage> decode_add_order_with_mpid_message(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] Result<OrderExecutedMessage> decode_order_executed_message(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] Result<OrderExecutedWithPriceMessage>
decode_order_executed_with_price_message(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] Result<OrderCancelMessage> decode_order_cancel_message(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] Result<OrderDeleteMessage> decode_order_delete_message(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] Result<OrderReplaceMessage> decode_order_replace_message(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] Result<TradeNonCrossMessage> decode_trade_non_cross_message(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] Result<CrossTradeMessage> decode_cross_trade_message(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] Result<BrokenTradeMessage> decode_broken_trade_message(
    std::span<const std::byte> bytes
) noexcept;

// -----------------------------------------------------------------------------
// Decode support checks
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr bool is_decode_supported(
    MessageType message_type
) noexcept {
    switch (message_type) {
        case MessageType::system_event:
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
            return true;

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
            return false;
    }

    return false;
}

} // namespace fgep::itch