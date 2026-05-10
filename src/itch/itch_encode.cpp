#include "fgep/itch/itch_encode.hpp"

#include "fgep/wire/byte_io.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <span>
#include <type_traits>
#include <variant>

namespace fgep::itch {
namespace {

[[nodiscard]] ErrorCode write_char(
    std::span<std::byte> bytes,
    std::size_t offset,
    char value
) noexcept {
    return wire::write_u8(
        bytes,
        offset,
        static_cast<std::uint8_t>(static_cast<unsigned char>(value))
    );
}

[[nodiscard]] ErrorCode require_valid_system_event_for_encode(
    const SystemEventMessage& message
) noexcept {
    if (message.header.stock_locate != 0) {
        return ErrorCode::invalid_argument;
    }

    if (!is_valid_timestamp_ns(message.header.timestamp_ns)) {
        return ErrorCode::invalid_argument;
    }

    return ErrorCode::ok;
}

[[nodiscard]] ErrorCode require_valid_stock_header_for_encode(
    const Header& header
) noexcept {
    if (!is_valid_header_for_stock_message(header)) {
        return ErrorCode::invalid_argument;
    }

    return ErrorCode::ok;
}

} // namespace

// -----------------------------------------------------------------------------
// Generic encode helpers
// -----------------------------------------------------------------------------

ErrorCode encode_message_type(
    std::span<std::byte> bytes,
    MessageType message_type
) noexcept {
    return write_char(bytes, offset_message_type, to_char(message_type));
}

ErrorCode encode_header(
    std::span<std::byte> bytes,
    const Header& header
) noexcept {
    if (!wire::has_range(bytes, 0, size_common_header)) {
        return ErrorCode::parse_error;
    }

    if (!is_valid_timestamp_ns(header.timestamp_ns)) {
        return ErrorCode::invalid_argument;
    }

    auto error = wire::write_u16_be(
        bytes,
        offset_stock_locate,
        header.stock_locate
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u16_be(
        bytes,
        offset_tracking_number,
        header.tracking_number
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_u48_be(bytes, offset_timestamp, header.timestamp_ns);
}

ErrorCode require_exact_encode_length(
    std::span<std::byte> bytes,
    MessageType message_type
) noexcept {
    if (!is_encode_supported(message_type)) {
        return ErrorCode::unsupported;
    }

    const auto expected_length = message_length(message_type);

    if (expected_length.failed()) {
        return expected_length.error;
    }

    if (bytes.size() != expected_length.value) {
        return ErrorCode::parse_error;
    }

    return ErrorCode::ok;
}

ErrorCode encode_message(
    std::span<std::byte> bytes,
    const Message& message
) noexcept {
    return std::visit(
        [&bytes](const auto& concrete_message) -> ErrorCode {
            using MessageT = std::decay_t<decltype(concrete_message)>;

            if constexpr (std::is_same_v<MessageT, SystemEventMessage>) {
                return encode_system_event_message(bytes, concrete_message);
            } else if constexpr (
                std::is_same_v<MessageT, AddOrderNoMpidMessage>
            ) {
                return encode_add_order_no_mpid_message(
                    bytes,
                    concrete_message
                );
            } else if constexpr (
                std::is_same_v<MessageT, AddOrderWithMpidMessage>
            ) {
                return encode_add_order_with_mpid_message(
                    bytes,
                    concrete_message
                );
            } else if constexpr (
                std::is_same_v<MessageT, OrderExecutedMessage>
            ) {
                return encode_order_executed_message(bytes, concrete_message);
            } else if constexpr (
                std::is_same_v<MessageT, OrderExecutedWithPriceMessage>
            ) {
                return encode_order_executed_with_price_message(
                    bytes,
                    concrete_message
                );
            } else if constexpr (
                std::is_same_v<MessageT, OrderCancelMessage>
            ) {
                return encode_order_cancel_message(bytes, concrete_message);
            } else if constexpr (
                std::is_same_v<MessageT, OrderDeleteMessage>
            ) {
                return encode_order_delete_message(bytes, concrete_message);
            } else if constexpr (
                std::is_same_v<MessageT, OrderReplaceMessage>
            ) {
                return encode_order_replace_message(bytes, concrete_message);
            } else if constexpr (
                std::is_same_v<MessageT, TradeNonCrossMessage>
            ) {
                return encode_trade_non_cross_message(bytes, concrete_message);
            } else if constexpr (
                std::is_same_v<MessageT, CrossTradeMessage>
            ) {
                return encode_cross_trade_message(bytes, concrete_message);
            } else if constexpr (
                std::is_same_v<MessageT, StockDirectoryMessage>
            ) {
                return encode_stock_directory_message(bytes, concrete_message);
            } else if constexpr (
                std::is_same_v<MessageT, StockTradingActionMessage>
            ) {
                return encode_stock_trading_action_message(bytes, concrete_message);            
            } else if constexpr (
                std::is_same_v<MessageT, BrokenTradeMessage>
            ) {
                return encode_broken_trade_message(bytes, concrete_message);
            } else {
                return ErrorCode::unsupported;
            }
        },
        message
    );
}

// -----------------------------------------------------------------------------
// Message-specific encoders
// -----------------------------------------------------------------------------

ErrorCode encode_system_event_message(
    std::span<std::byte> bytes,
    const SystemEventMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::system_event
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = require_valid_system_event_for_encode(message);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::system_event);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    return write_char(
        bytes,
        system_event_offset_event_code,
        to_char(message.event_code)
    );
}

ErrorCode encode_stock_directory_message(
    std::span<std::byte> bytes,
    const StockDirectoryMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::stock_directory
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_stock_directory_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::stock_directory);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_fixed_ascii<8>(
        bytes,
        stock_directory_offset_stock,
        message.stock
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        stock_directory_offset_market_category,
        message.market_category
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        stock_directory_offset_financial_status_indicator,
        message.financial_status_indicator
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        stock_directory_offset_round_lot_size,
        message.round_lot_size
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        stock_directory_offset_round_lots_only,
        message.round_lots_only
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        stock_directory_offset_issue_classification,
        message.issue_classification
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_fixed_ascii<2>(
        bytes,
        stock_directory_offset_issue_sub_type,
        message.issue_sub_type
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        stock_directory_offset_authenticity,
        message.authenticity
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        stock_directory_offset_short_sale_threshold_indicator,
        message.short_sale_threshold_indicator
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        stock_directory_offset_ipo_flag,
        message.ipo_flag
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        stock_directory_offset_luld_reference_price_tier,
        message.luld_reference_price_tier
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        stock_directory_offset_etp_flag,
        message.etp_flag
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        stock_directory_offset_etp_leverage_factor,
        message.etp_leverage_factor
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return write_char(
        bytes,
        stock_directory_offset_inverse_indicator,
        message.inverse_indicator
    );
}

ErrorCode encode_stock_trading_action_message(
    std::span<std::byte> bytes,
    const StockTradingActionMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::stock_trading_action
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_stock_trading_action_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::stock_trading_action);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_fixed_ascii<8>(
        bytes,
        stock_trading_action_offset_stock,
        message.stock
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        stock_trading_action_offset_trading_state,
        to_char(message.trading_state)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        stock_trading_action_offset_reserved,
        message.reserved
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_fixed_ascii<4>(
        bytes,
        stock_trading_action_offset_reason,
        message.reason
    );
}

ErrorCode encode_add_order_no_mpid_message(
    std::span<std::byte> bytes,
    const AddOrderNoMpidMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::add_order_no_mpid
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_add_order_no_mpid_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::add_order_no_mpid);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        add_order_offset_order_reference_number,
        message.order_reference_number
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(bytes, add_order_offset_side, to_char(message.side));

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        add_order_offset_shares,
        message.shares
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_fixed_ascii<8>(
        bytes,
        add_order_offset_stock,
        message.stock
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_u32_be(bytes, add_order_offset_price, message.price);
}

ErrorCode encode_add_order_with_mpid_message(
    std::span<std::byte> bytes,
    const AddOrderWithMpidMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::add_order_with_mpid
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_add_order_with_mpid_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::add_order_with_mpid);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        add_order_with_mpid_offset_order_reference_number,
        message.order_reference_number
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        add_order_with_mpid_offset_side,
        to_char(message.side)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        add_order_with_mpid_offset_shares,
        message.shares
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_fixed_ascii<8>(
        bytes,
        add_order_with_mpid_offset_stock,
        message.stock
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        add_order_with_mpid_offset_price,
        message.price
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_fixed_ascii<4>(
        bytes,
        add_order_with_mpid_offset_attribution,
        message.attribution
    );
}

ErrorCode encode_order_executed_message(
    std::span<std::byte> bytes,
    const OrderExecutedMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::order_executed
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_order_executed_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::order_executed);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        order_executed_offset_order_reference_number,
        message.order_reference_number
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        order_executed_offset_executed_shares,
        message.executed_shares
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_u64_be(
        bytes,
        order_executed_offset_match_number,
        message.match_number
    );
}

ErrorCode encode_order_executed_with_price_message(
    std::span<std::byte> bytes,
    const OrderExecutedWithPriceMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::order_executed_with_price
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_order_executed_with_price_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(
        bytes,
        MessageType::order_executed_with_price
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        order_executed_with_price_offset_order_reference_number,
        message.order_reference_number
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        order_executed_with_price_offset_executed_shares,
        message.executed_shares
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        order_executed_with_price_offset_match_number,
        message.match_number
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        order_executed_with_price_offset_printable,
        to_char(message.printable)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_u32_be(
        bytes,
        order_executed_with_price_offset_execution_price,
        message.execution_price
    );
}

ErrorCode encode_order_cancel_message(
    std::span<std::byte> bytes,
    const OrderCancelMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::order_cancel
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_order_cancel_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::order_cancel);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        order_cancel_offset_order_reference_number,
        message.order_reference_number
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_u32_be(
        bytes,
        order_cancel_offset_cancelled_shares,
        message.cancelled_shares
    );
}

ErrorCode encode_order_delete_message(
    std::span<std::byte> bytes,
    const OrderDeleteMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::order_delete
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_order_delete_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::order_delete);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_u64_be(
        bytes,
        order_delete_offset_order_reference_number,
        message.order_reference_number
    );
}

ErrorCode encode_order_replace_message(
    std::span<std::byte> bytes,
    const OrderReplaceMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::order_replace
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_order_replace_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::order_replace);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        order_replace_offset_original_order_reference_number,
        message.original_order_reference_number
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        order_replace_offset_new_order_reference_number,
        message.new_order_reference_number
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        order_replace_offset_shares,
        message.shares
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_u32_be(
        bytes,
        order_replace_offset_price,
        message.price
    );
}

ErrorCode encode_trade_non_cross_message(
    std::span<std::byte> bytes,
    const TradeNonCrossMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::trade_non_cross
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_trade_non_cross_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::trade_non_cross);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        trade_non_cross_offset_order_reference_number,
        message.order_reference_number
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        trade_non_cross_offset_side,
        to_char(message.side)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        trade_non_cross_offset_shares,
        message.shares
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_fixed_ascii<8>(
        bytes,
        trade_non_cross_offset_stock,
        message.stock
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        trade_non_cross_offset_price,
        message.price
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_u64_be(
        bytes,
        trade_non_cross_offset_match_number,
        message.match_number
    );
}

ErrorCode encode_cross_trade_message(
    std::span<std::byte> bytes,
    const CrossTradeMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::cross_trade
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_cross_trade_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::cross_trade);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        cross_trade_offset_shares,
        message.shares
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_fixed_ascii<8>(
        bytes,
        cross_trade_offset_stock,
        message.stock
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        cross_trade_offset_cross_price,
        message.cross_price
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        cross_trade_offset_match_number,
        message.match_number
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return write_char(
        bytes,
        cross_trade_offset_cross_type,
        to_char(message.cross_type)
    );
}

ErrorCode encode_broken_trade_message(
    std::span<std::byte> bytes,
    const BrokenTradeMessage& message
) noexcept {
    auto error = require_exact_encode_length(
        bytes,
        MessageType::broken_trade
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_broken_trade_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = require_valid_stock_header_for_encode(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_message_type(bytes, MessageType::broken_trade);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = encode_header(bytes, message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_u64_be(
        bytes,
        broken_trade_offset_match_number,
        message.match_number
    );
}

} // namespace fgep::itchc