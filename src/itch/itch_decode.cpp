#include "fgep/itch/itch_decode.hpp"

#include "fgep/wire/byte_io.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cstddef>
#include <span>

namespace fgep::itch {
namespace {

[[nodiscard]] Result<char> read_char(
    std::span<const std::byte> bytes,
    std::size_t offset
) noexcept {
    const auto result = wire::read_u8(bytes, offset);

    if (result.failed()) {
        return {result.error, '\0'};
    }

    return {ErrorCode::ok, static_cast<char>(result.value)};
}

[[nodiscard]] ErrorCode require_valid_header_for_stock_message(
    const Header& header
) noexcept {
    if (!is_valid_header_for_stock_message(header)) {
        return ErrorCode::parse_error;
    }

    return ErrorCode::ok;
}

[[nodiscard]] ErrorCode require_valid_system_event_header(
    const Header& header
) noexcept {
    // Nasdaq ITCH System Event messages are not stock-specific.
    // The specification marks Stock Locate as always 0 for this message.
    if (header.stock_locate != 0) {
        return ErrorCode::parse_error;
    }

    if (!is_valid_timestamp_ns(header.timestamp_ns)) {
        return ErrorCode::parse_error;
    }

    return ErrorCode::ok;
}

} // namespace

// -----------------------------------------------------------------------------
// Generic decode helpers
// -----------------------------------------------------------------------------

Result<CrossTradeMessage> decode_cross_trade_message(
    std::span<const std::byte> bytes
) noexcept {
    CrossTradeMessage message{};

    const auto type_and_length_error = require_exact_message_type_and_length(
        bytes,
        MessageType::cross_trade
    );

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error =
        require_valid_header_for_stock_message(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto shares =
        wire::read_u64_be(bytes, cross_trade_offset_shares);

    if (shares.failed()) {
        return {shares.error, message};
    }

    const auto stock =
        wire::read_fixed_ascii<8>(bytes, cross_trade_offset_stock);

    if (stock.failed()) {
        return {stock.error, message};
    }

    const auto cross_price =
        wire::read_u32_be(bytes, cross_trade_offset_cross_price);

    if (cross_price.failed()) {
        return {cross_price.error, message};
    }

    const auto match_number =
        wire::read_u64_be(bytes, cross_trade_offset_match_number);

    if (match_number.failed()) {
        return {match_number.error, message};
    }

    const auto cross_type =
        read_char(bytes, cross_trade_offset_cross_type);

    if (cross_type.failed()) {
        return {cross_type.error, message};
    }

    const auto parsed_cross_type = cross_type_from_char(cross_type.value);

    if (parsed_cross_type.failed()) {
        return {parsed_cross_type.error, message};
    }

    message.header = header.value;
    message.shares = shares.value;
    message.stock = stock.value;
    message.cross_price = cross_price.value;
    message.match_number = match_number.value;
    message.cross_type = parsed_cross_type.value;

    if (!is_valid_cross_trade_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<Header> decode_header(std::span<const std::byte> bytes) noexcept {
    Header header{};

    if (!wire::has_range(bytes, 0, size_common_header)) {
        return {ErrorCode::parse_error, header};
    }

    const auto stock_locate =
        wire::read_u16_be(bytes, offset_stock_locate);

    if (stock_locate.failed()) {
        return {stock_locate.error, header};
    }

    const auto tracking_number =
        wire::read_u16_be(bytes, offset_tracking_number);

    if (tracking_number.failed()) {
        return {tracking_number.error, header};
    }

    const auto timestamp =
        wire::read_u48_be(bytes, offset_timestamp);

    if (timestamp.failed()) {
        return {timestamp.error, header};
    }

    header.stock_locate = stock_locate.value;
    header.tracking_number = tracking_number.value;
    header.timestamp_ns = timestamp.value;

    if (!is_valid_timestamp_ns(header.timestamp_ns)) {
        return {ErrorCode::parse_error, header};
    }

    return {ErrorCode::ok, header};
}

ErrorCode require_exact_message_type_and_length(
    std::span<const std::byte> bytes,
    MessageType expected_message_type
) noexcept {
    const auto decoded_message_type = decode_message_type(bytes);

    if (decoded_message_type.failed()) {
        return decoded_message_type.error;
    }

    if (decoded_message_type.value != expected_message_type) {
        return ErrorCode::parse_error;
    }

    const auto expected_length = message_length(expected_message_type);

    if (expected_length.failed()) {
        return expected_length.error;
    }

    if (bytes.size() != expected_length.value) {
        return ErrorCode::parse_error;
    }

    return ErrorCode::ok;
}

Result<Message> decode_message(std::span<const std::byte> bytes) noexcept {
    const auto decoded_message_type = decode_message_type(bytes);

    if (decoded_message_type.failed()) {
        return {decoded_message_type.error, Message{}};
    }

    switch (decoded_message_type.value) {
        case MessageType::system_event: {
            const auto result = decode_system_event_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::add_order_no_mpid: {
            const auto result = decode_add_order_no_mpid_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::add_order_with_mpid: {
            const auto result = decode_add_order_with_mpid_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::order_executed: {
            const auto result = decode_order_executed_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::order_executed_with_price: {
            const auto result =
                decode_order_executed_with_price_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::order_cancel: {
            const auto result = decode_order_cancel_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::order_delete: {
            const auto result = decode_order_delete_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::order_replace: {
            const auto result = decode_order_replace_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::trade_non_cross: {
            const auto result = decode_trade_non_cross_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::cross_trade: {
            const auto result = decode_cross_trade_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::broken_trade: {
            const auto result = decode_broken_trade_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

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
            return {ErrorCode::unsupported, Message{}};
    }

    return {ErrorCode::unsupported, Message{}};
}

// -----------------------------------------------------------------------------
// Message-specific decoders
// -----------------------------------------------------------------------------

Result<SystemEventMessage> decode_system_event_message(
    std::span<const std::byte> bytes
) noexcept {
    SystemEventMessage message{};

    const auto type_and_length_error =
        require_exact_message_type_and_length(bytes, MessageType::system_event);

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error = require_valid_system_event_header(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto event_code =
        read_char(bytes, system_event_offset_event_code);

    if (event_code.failed()) {
        return {event_code.error, message};
    }

    const auto parsed_event_code =
        system_event_code_from_char(event_code.value);

    if (parsed_event_code.failed()) {
        return {parsed_event_code.error, message};
    }

    message.header = header.value;
    message.event_code = parsed_event_code.value;

    return {ErrorCode::ok, message};
}

Result<AddOrderNoMpidMessage> decode_add_order_no_mpid_message(
    std::span<const std::byte> bytes
) noexcept {
    AddOrderNoMpidMessage message{};

    const auto type_and_length_error = require_exact_message_type_and_length(
        bytes,
        MessageType::add_order_no_mpid
    );

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error =
        require_valid_header_for_stock_message(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto order_reference_number = wire::read_u64_be(
        bytes,
        add_order_offset_order_reference_number
    );

    if (order_reference_number.failed()) {
        return {order_reference_number.error, message};
    }

    const auto side = read_char(bytes, add_order_offset_side);

    if (side.failed()) {
        return {side.error, message};
    }

    const auto parsed_side = side_from_char(side.value);

    if (parsed_side.failed()) {
        return {parsed_side.error, message};
    }

    const auto shares = wire::read_u32_be(bytes, add_order_offset_shares);

    if (shares.failed()) {
        return {shares.error, message};
    }

    const auto stock =
        wire::read_fixed_ascii<8>(bytes, add_order_offset_stock);

    if (stock.failed()) {
        return {stock.error, message};
    }

    const auto price = wire::read_u32_be(bytes, add_order_offset_price);

    if (price.failed()) {
        return {price.error, message};
    }

    message.header = header.value;
    message.order_reference_number = order_reference_number.value;
    message.side = parsed_side.value;
    message.shares = shares.value;
    message.stock = stock.value;
    message.price = price.value;

    if (!is_valid_add_order_no_mpid_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<AddOrderWithMpidMessage> decode_add_order_with_mpid_message(
    std::span<const std::byte> bytes
) noexcept {
    AddOrderWithMpidMessage message{};

    const auto type_and_length_error = require_exact_message_type_and_length(
        bytes,
        MessageType::add_order_with_mpid
    );

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error =
        require_valid_header_for_stock_message(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto order_reference_number = wire::read_u64_be(
        bytes,
        add_order_with_mpid_offset_order_reference_number
    );

    if (order_reference_number.failed()) {
        return {order_reference_number.error, message};
    }

    const auto side = read_char(bytes, add_order_with_mpid_offset_side);

    if (side.failed()) {
        return {side.error, message};
    }

    const auto parsed_side = side_from_char(side.value);

    if (parsed_side.failed()) {
        return {parsed_side.error, message};
    }

    const auto shares =
        wire::read_u32_be(bytes, add_order_with_mpid_offset_shares);

    if (shares.failed()) {
        return {shares.error, message};
    }

    const auto stock =
        wire::read_fixed_ascii<8>(bytes, add_order_with_mpid_offset_stock);

    if (stock.failed()) {
        return {stock.error, message};
    }

    const auto price =
        wire::read_u32_be(bytes, add_order_with_mpid_offset_price);

    if (price.failed()) {
        return {price.error, message};
    }

    const auto attribution =
        wire::read_fixed_ascii<4>(bytes, add_order_with_mpid_offset_attribution);

    if (attribution.failed()) {
        return {attribution.error, message};
    }

    message.header = header.value;
    message.order_reference_number = order_reference_number.value;
    message.side = parsed_side.value;
    message.shares = shares.value;
    message.stock = stock.value;
    message.price = price.value;
    message.attribution = attribution.value;

    if (!is_valid_add_order_with_mpid_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<OrderExecutedMessage> decode_order_executed_message(
    std::span<const std::byte> bytes
) noexcept {
    OrderExecutedMessage message{};

    const auto type_and_length_error = require_exact_message_type_and_length(
        bytes,
        MessageType::order_executed
    );

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error =
        require_valid_header_for_stock_message(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto order_reference_number = wire::read_u64_be(
        bytes,
        order_executed_offset_order_reference_number
    );

    if (order_reference_number.failed()) {
        return {order_reference_number.error, message};
    }

    const auto executed_shares =
        wire::read_u32_be(bytes, order_executed_offset_executed_shares);

    if (executed_shares.failed()) {
        return {executed_shares.error, message};
    }

    const auto match_number =
        wire::read_u64_be(bytes, order_executed_offset_match_number);

    if (match_number.failed()) {
        return {match_number.error, message};
    }

    message.header = header.value;
    message.order_reference_number = order_reference_number.value;
    message.executed_shares = executed_shares.value;
    message.match_number = match_number.value;

    if (!is_valid_order_executed_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<OrderExecutedWithPriceMessage>
decode_order_executed_with_price_message(
    std::span<const std::byte> bytes
) noexcept {
    OrderExecutedWithPriceMessage message{};

    const auto type_and_length_error = require_exact_message_type_and_length(
        bytes,
        MessageType::order_executed_with_price
    );

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error =
        require_valid_header_for_stock_message(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto order_reference_number = wire::read_u64_be(
        bytes,
        order_executed_with_price_offset_order_reference_number
    );

    if (order_reference_number.failed()) {
        return {order_reference_number.error, message};
    }

    const auto executed_shares = wire::read_u32_be(
        bytes,
        order_executed_with_price_offset_executed_shares
    );

    if (executed_shares.failed()) {
        return {executed_shares.error, message};
    }

    const auto match_number = wire::read_u64_be(
        bytes,
        order_executed_with_price_offset_match_number
    );

    if (match_number.failed()) {
        return {match_number.error, message};
    }

    const auto printable =
        read_char(bytes, order_executed_with_price_offset_printable);

    if (printable.failed()) {
        return {printable.error, message};
    }

    const auto parsed_printable = printable_from_char(printable.value);

    if (parsed_printable.failed()) {
        return {parsed_printable.error, message};
    }

    const auto execution_price =
        wire::read_u32_be(bytes, order_executed_with_price_offset_execution_price);

    if (execution_price.failed()) {
        return {execution_price.error, message};
    }

    message.header = header.value;
    message.order_reference_number = order_reference_number.value;
    message.executed_shares = executed_shares.value;
    message.match_number = match_number.value;
    message.printable = parsed_printable.value;
    message.execution_price = execution_price.value;

    if (!is_valid_order_executed_with_price_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<OrderCancelMessage> decode_order_cancel_message(
    std::span<const std::byte> bytes
) noexcept {
    OrderCancelMessage message{};

    const auto type_and_length_error = require_exact_message_type_and_length(
        bytes,
        MessageType::order_cancel
    );

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error =
        require_valid_header_for_stock_message(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto order_reference_number =
        wire::read_u64_be(bytes, order_cancel_offset_order_reference_number);

    if (order_reference_number.failed()) {
        return {order_reference_number.error, message};
    }

    const auto cancelled_shares =
        wire::read_u32_be(bytes, order_cancel_offset_cancelled_shares);

    if (cancelled_shares.failed()) {
        return {cancelled_shares.error, message};
    }

    message.header = header.value;
    message.order_reference_number = order_reference_number.value;
    message.cancelled_shares = cancelled_shares.value;

    if (!is_valid_order_cancel_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<OrderDeleteMessage> decode_order_delete_message(
    std::span<const std::byte> bytes
) noexcept {
    OrderDeleteMessage message{};

    const auto type_and_length_error = require_exact_message_type_and_length(
        bytes,
        MessageType::order_delete
    );

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error =
        require_valid_header_for_stock_message(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto order_reference_number =
        wire::read_u64_be(bytes, order_delete_offset_order_reference_number);

    if (order_reference_number.failed()) {
        return {order_reference_number.error, message};
    }

    message.header = header.value;
    message.order_reference_number = order_reference_number.value;

    if (!is_valid_order_delete_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<OrderReplaceMessage> decode_order_replace_message(
    std::span<const std::byte> bytes
) noexcept {
    OrderReplaceMessage message{};

    const auto type_and_length_error = require_exact_message_type_and_length(
        bytes,
        MessageType::order_replace
    );

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error =
        require_valid_header_for_stock_message(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto original_order_reference_number = wire::read_u64_be(
        bytes,
        order_replace_offset_original_order_reference_number
    );

    if (original_order_reference_number.failed()) {
        return {original_order_reference_number.error, message};
    }

    const auto new_order_reference_number = wire::read_u64_be(
        bytes,
        order_replace_offset_new_order_reference_number
    );

    if (new_order_reference_number.failed()) {
        return {new_order_reference_number.error, message};
    }

    const auto shares =
        wire::read_u32_be(bytes, order_replace_offset_shares);

    if (shares.failed()) {
        return {shares.error, message};
    }

    const auto price =
        wire::read_u32_be(bytes, order_replace_offset_price);

    if (price.failed()) {
        return {price.error, message};
    }

    message.header = header.value;
    message.original_order_reference_number =
        original_order_reference_number.value;
    message.new_order_reference_number = new_order_reference_number.value;
    message.shares = shares.value;
    message.price = price.value;

    if (!is_valid_order_replace_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<TradeNonCrossMessage> decode_trade_non_cross_message(
    std::span<const std::byte> bytes
) noexcept {
    TradeNonCrossMessage message{};

    const auto type_and_length_error = require_exact_message_type_and_length(
        bytes,
        MessageType::trade_non_cross
    );

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error =
        require_valid_header_for_stock_message(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto order_reference_number = wire::read_u64_be(
        bytes,
        trade_non_cross_offset_order_reference_number
    );

    if (order_reference_number.failed()) {
        return {order_reference_number.error, message};
    }

    const auto side = read_char(bytes, trade_non_cross_offset_side);

    if (side.failed()) {
        return {side.error, message};
    }

    const auto parsed_side = side_from_char(side.value);

    if (parsed_side.failed()) {
        return {parsed_side.error, message};
    }

    const auto shares =
        wire::read_u32_be(bytes, trade_non_cross_offset_shares);

    if (shares.failed()) {
        return {shares.error, message};
    }

    const auto stock =
        wire::read_fixed_ascii<8>(bytes, trade_non_cross_offset_stock);

    if (stock.failed()) {
        return {stock.error, message};
    }

    const auto price =
        wire::read_u32_be(bytes, trade_non_cross_offset_price);

    if (price.failed()) {
        return {price.error, message};
    }

    const auto match_number =
        wire::read_u64_be(bytes, trade_non_cross_offset_match_number);

    if (match_number.failed()) {
        return {match_number.error, message};
    }

    message.header = header.value;
    message.order_reference_number = order_reference_number.value;
    message.side = parsed_side.value;
    message.shares = shares.value;
    message.stock = stock.value;
    message.price = price.value;
    message.match_number = match_number.value;

    if (!is_valid_trade_non_cross_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<CrossTradeMessage> decode_cross_trade_message(
    std::span<const std::byte> bytes
) noexcept {
    CrossTradeMessage message{};

    const auto type_and_length_error = require_exact_message_type_and_length(
        bytes,
        MessageType::cross_trade
    );

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error =
        require_valid_header_for_stock_message(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto shares =
        wire::read_u64_be(bytes, cross_trade_offset_shares);

    if (shares.failed()) {
        return {shares.error, message};
    }

    if (shares.value > 0xFFFFFFFFULL) {
        return {ErrorCode::invalid_argument, message};
    }

    const auto stock =
        wire::read_fixed_ascii<8>(bytes, cross_trade_offset_stock);

    if (stock.failed()) {
        return {stock.error, message};
    }

    const auto cross_price =
        wire::read_u32_be(bytes, cross_trade_offset_cross_price);

    if (cross_price.failed()) {
        return {cross_price.error, message};
    }

    const auto match_number =
        wire::read_u64_be(bytes, cross_trade_offset_match_number);

    if (match_number.failed()) {
        return {match_number.error, message};
    }

    const auto cross_type =
        read_char(bytes, cross_trade_offset_cross_type);

    if (cross_type.failed()) {
        return {cross_type.error, message};
    }

    const auto parsed_cross_type = cross_type_from_char(cross_type.value);

    if (parsed_cross_type.failed()) {
        return {parsed_cross_type.error, message};
    }

    message.header = header.value;
    message.shares = static_cast<Shares>(shares.value);
    message.stock = stock.value;
    message.cross_price = cross_price.value;
    message.match_number = match_number.value;
    message.cross_type = parsed_cross_type.value;

    if (!is_valid_cross_trade_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<BrokenTradeMessage> decode_broken_trade_message(
    std::span<const std::byte> bytes
) noexcept {
    BrokenTradeMessage message{};

    const auto type_and_length_error = require_exact_message_type_and_length(
        bytes,
        MessageType::broken_trade
    );

    if (type_and_length_error != ErrorCode::ok) {
        return {type_and_length_error, message};
    }

    const auto header = decode_header(bytes);

    if (header.failed()) {
        return {header.error, message};
    }

    const auto header_error =
        require_valid_header_for_stock_message(header.value);

    if (header_error != ErrorCode::ok) {
        return {header_error, message};
    }

    const auto match_number =
        wire::read_u64_be(bytes, broken_trade_offset_match_number);

    if (match_number.failed()) {
        return {match_number.error, message};
    }

    message.header = header.value;
    message.match_number = match_number.value;

    if (!is_valid_broken_trade_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

} // namespace fgep::itch