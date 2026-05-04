#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/itch/itch_message_type.hpp"
#include "fgep/itch/itch_wire_messages.hpp"

#include <cstddef>
#include <span>

namespace fgep::itch {

// -----------------------------------------------------------------------------
// Nasdaq TotalView-ITCH 5.0 byte encoder API
// -----------------------------------------------------------------------------
//
// This header declares the message-to-byte encode functions for ITCH payload
// messages.
//
// This is the byte-to-byte protocol layer:
//
//   decoded itch::*Message struct
//   -> explicit offset writes
//   -> raw ITCH payload bytes
//
// Important:
//   These functions encode one ITCH payload message at a time.
//   They do not encode SoupBinTCP framing.
//   They do not encode MoldUDP64 packet framing.
//   They do not reinterpret structs as raw bytes.
//
// Transport framing should be handled by a separate module later:
//
//   soupbintcp/
//   moldudp64/
//
// The output span must point at the start of the ITCH payload:
//
//   bytes[0] = ITCH Message Type
//
// Byte-to-byte tests should verify:
//
//   original bytes
//   -> decode_message(original bytes)
//   -> encode_message(decoded message)
//   -> exact same bytes

// -----------------------------------------------------------------------------
// Generic encode helpers
// -----------------------------------------------------------------------------

[[nodiscard]] ErrorCode encode_message_type(
    std::span<std::byte> bytes,
    MessageType message_type
) noexcept;

[[nodiscard]] ErrorCode encode_header(
    std::span<std::byte> bytes,
    const Header& header
) noexcept;

// Checks that:
//   1. bytes.size() equals the exact expected message length
//   2. the message type is supported for encoding
//
// This is strict on purpose. Byte-to-byte encode should not silently leave extra
// bytes at the end of a buffer.
[[nodiscard]] ErrorCode require_exact_encode_length(
    std::span<std::byte> bytes,
    MessageType message_type
) noexcept;

// Dispatches by variant alternative and encodes one supported ITCH message.
[[nodiscard]] ErrorCode encode_message(
    std::span<std::byte> bytes,
    const Message& message
) noexcept;

// -----------------------------------------------------------------------------
// Message-specific encoders
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

[[nodiscard]] ErrorCode encode_system_event_message(
    std::span<std::byte> bytes,
    const SystemEventMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_add_order_no_mpid_message(
    std::span<std::byte> bytes,
    const AddOrderNoMpidMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_add_order_with_mpid_message(
    std::span<std::byte> bytes,
    const AddOrderWithMpidMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_order_executed_message(
    std::span<std::byte> bytes,
    const OrderExecutedMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_order_executed_with_price_message(
    std::span<std::byte> bytes,
    const OrderExecutedWithPriceMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_order_cancel_message(
    std::span<std::byte> bytes,
    const OrderCancelMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_order_delete_message(
    std::span<std::byte> bytes,
    const OrderDeleteMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_order_replace_message(
    std::span<std::byte> bytes,
    const OrderReplaceMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_trade_non_cross_message(
    std::span<std::byte> bytes,
    const TradeNonCrossMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_cross_trade_message(
    std::span<std::byte> bytes,
    const CrossTradeMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_broken_trade_message(
    std::span<std::byte> bytes,
    const BrokenTradeMessage& message
) noexcept;

// -----------------------------------------------------------------------------
// Encode support checks
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr bool is_encode_supported(
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