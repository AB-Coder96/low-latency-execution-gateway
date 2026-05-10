#include "fgep/ouch/ouch_decode.hpp"

#include "fgep/wire/byte_io.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cstddef>
#include <span>

namespace fgep::ouch {
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

[[nodiscard]] ErrorCode require_minimum_length(
    std::span<const std::byte> bytes,
    std::size_t minimum_length
) noexcept {
    if (bytes.size() < minimum_length) {
        return ErrorCode::parse_error;
    }

    return ErrorCode::ok;
}

[[nodiscard]] ErrorCode require_appendage_exact_length(
    std::span<const std::byte> bytes,
    std::size_t base_length,
    AppendageLength appendage_length
) noexcept {
    const auto expected_length =
        base_length + static_cast<std::size_t>(appendage_length);

    if (bytes.size() != expected_length) {
        return ErrorCode::parse_error;
    }

    return ErrorCode::ok;
}

} // namespace

Result<MessageType> decode_message_type(
    std::span<const std::byte> bytes
) noexcept {
    const auto type = read_char(bytes, 0);

    if (type.failed()) {
        return {type.error, MessageType::enter_order};
    }

    return message_type_from_char(type.value);
}

ErrorCode require_exact_message_type(
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

    return ErrorCode::ok;
}

Result<Message> decode_message(std::span<const std::byte> bytes) {
    const auto decoded_message_type = decode_message_type(bytes);

    if (decoded_message_type.failed()) {
        return {decoded_message_type.error, Message{}};
    }

    switch (decoded_message_type.value) {
        case MessageType::enter_order: {
            const auto result = decode_enter_order_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::replace_order: {
            const auto result = decode_replace_order_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }

        case MessageType::cancel_order: {
            const auto result = decode_cancel_order_message(bytes);

            if (result.failed()) {
                return {result.error, Message{}};
            }

            return {ErrorCode::ok, Message{result.value}};
        }
    }

    return {ErrorCode::unsupported, Message{}};
}

Result<EnterOrderMessage> decode_enter_order_message(
    std::span<const std::byte> bytes
) {
    EnterOrderMessage message{};

    auto error = require_exact_message_type(bytes, MessageType::enter_order);

    if (error != ErrorCode::ok) {
        return {error, message};
    }

    error = require_minimum_length(bytes, length_enter_order_base);

    if (error != ErrorCode::ok) {
        return {error, message};
    }

    const auto user_ref_num = wire::read_u32_be(
        bytes,
        enter_order_offset_user_ref_num
    );

    if (user_ref_num.failed()) {
        return {user_ref_num.error, message};
    }

    const auto side = read_char(bytes, enter_order_offset_side);

    if (side.failed()) {
        return {side.error, message};
    }

    const auto parsed_side = side_from_char(side.value);

    if (parsed_side.failed()) {
        return {parsed_side.error, message};
    }

    const auto quantity = wire::read_u32_be(
        bytes,
        enter_order_offset_quantity
    );

    if (quantity.failed()) {
        return {quantity.error, message};
    }

    const auto symbol = wire::read_fixed_ascii<8>(
        bytes,
        enter_order_offset_symbol
    );

    if (symbol.failed()) {
        return {symbol.error, message};
    }

    const auto price = wire::read_u64_be(bytes, enter_order_offset_price);

    if (price.failed()) {
        return {price.error, message};
    }

    const auto time_in_force = read_char(
        bytes,
        enter_order_offset_time_in_force
    );

    if (time_in_force.failed()) {
        return {time_in_force.error, message};
    }

    const auto parsed_time_in_force = time_in_force_from_char(
        time_in_force.value
    );

    if (parsed_time_in_force.failed()) {
        return {parsed_time_in_force.error, message};
    }

    const auto display = read_char(bytes, enter_order_offset_display);

    if (display.failed()) {
        return {display.error, message};
    }

    const auto parsed_display = display_from_char(display.value);

    if (parsed_display.failed()) {
        return {parsed_display.error, message};
    }

    const auto capacity = read_char(bytes, enter_order_offset_capacity);

    if (capacity.failed()) {
        return {capacity.error, message};
    }

    const auto parsed_capacity = capacity_from_char(capacity.value);

    if (parsed_capacity.failed()) {
        return {parsed_capacity.error, message};
    }

    const auto intermarket_sweep_eligibility = read_char(
        bytes,
        enter_order_offset_intermarket_sweep_eligibility
    );

    if (intermarket_sweep_eligibility.failed()) {
        return {intermarket_sweep_eligibility.error, message};
    }

    const auto parsed_intermarket_sweep_eligibility =
        intermarket_sweep_eligibility_from_char(
            intermarket_sweep_eligibility.value
        );

    if (parsed_intermarket_sweep_eligibility.failed()) {
        return {parsed_intermarket_sweep_eligibility.error, message};
    }

    const auto cross_type = read_char(bytes, enter_order_offset_cross_type);

    if (cross_type.failed()) {
        return {cross_type.error, message};
    }

    const auto parsed_cross_type = cross_type_from_char(cross_type.value);

    if (parsed_cross_type.failed()) {
        return {parsed_cross_type.error, message};
    }

    const auto cl_ord_id = wire::read_fixed_ascii<14>(
        bytes,
        enter_order_offset_cl_ord_id
    );

    if (cl_ord_id.failed()) {
        return {cl_ord_id.error, message};
    }

    const auto appendage_length = wire::read_u16_be(
        bytes,
        enter_order_offset_appendage_length
    );

    if (appendage_length.failed()) {
        return {appendage_length.error, message};
    }

    error = require_appendage_exact_length(
        bytes,
        length_enter_order_base,
        appendage_length.value
    );

    if (error != ErrorCode::ok) {
        return {error, message};
    }

    message.user_ref_num = user_ref_num.value;
    message.side = parsed_side.value;
    message.quantity = quantity.value;
    message.symbol = symbol.value;
    message.price = price.value;
    message.time_in_force = parsed_time_in_force.value;
    message.display = parsed_display.value;
    message.capacity = parsed_capacity.value;
    message.intermarket_sweep_eligibility =
        parsed_intermarket_sweep_eligibility.value;
    message.cross_type = parsed_cross_type.value;
    message.cl_ord_id = cl_ord_id.value;
    message.optional_appendage.assign(
        bytes.begin() + static_cast<std::ptrdiff_t>(enter_order_offset_optional_appendage),
        bytes.end()
    );

    if (!is_valid_enter_order_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<ReplaceOrderMessage> decode_replace_order_message(
    std::span<const std::byte> bytes
) {
    ReplaceOrderMessage message{};

    auto error = require_exact_message_type(bytes, MessageType::replace_order);

    if (error != ErrorCode::ok) {
        return {error, message};
    }

    error = require_minimum_length(bytes, length_replace_order_base);

    if (error != ErrorCode::ok) {
        return {error, message};
    }

    const auto orig_user_ref_num = wire::read_u32_be(
        bytes,
        replace_order_offset_orig_user_ref_num
    );

    if (orig_user_ref_num.failed()) {
        return {orig_user_ref_num.error, message};
    }

    const auto user_ref_num = wire::read_u32_be(
        bytes,
        replace_order_offset_user_ref_num
    );

    if (user_ref_num.failed()) {
        return {user_ref_num.error, message};
    }

    const auto quantity = wire::read_u32_be(
        bytes,
        replace_order_offset_quantity
    );

    if (quantity.failed()) {
        return {quantity.error, message};
    }

    const auto price = wire::read_u64_be(bytes, replace_order_offset_price);

    if (price.failed()) {
        return {price.error, message};
    }

    const auto time_in_force = read_char(
        bytes,
        replace_order_offset_time_in_force
    );

    if (time_in_force.failed()) {
        return {time_in_force.error, message};
    }

    const auto parsed_time_in_force = time_in_force_from_char(
        time_in_force.value
    );

    if (parsed_time_in_force.failed()) {
        return {parsed_time_in_force.error, message};
    }

    const auto display = read_char(bytes, replace_order_offset_display);

    if (display.failed()) {
        return {display.error, message};
    }

    const auto parsed_display = display_from_char(display.value);

    if (parsed_display.failed()) {
        return {parsed_display.error, message};
    }

    const auto intermarket_sweep_eligibility = read_char(
        bytes,
        replace_order_offset_intermarket_sweep_eligibility
    );

    if (intermarket_sweep_eligibility.failed()) {
        return {intermarket_sweep_eligibility.error, message};
    }

    const auto parsed_intermarket_sweep_eligibility =
        intermarket_sweep_eligibility_from_char(
            intermarket_sweep_eligibility.value
        );

    if (parsed_intermarket_sweep_eligibility.failed()) {
        return {parsed_intermarket_sweep_eligibility.error, message};
    }

    const auto cl_ord_id = wire::read_fixed_ascii<14>(
        bytes,
        replace_order_offset_cl_ord_id
    );

    if (cl_ord_id.failed()) {
        return {cl_ord_id.error, message};
    }

    const auto appendage_length = wire::read_u16_be(
        bytes,
        replace_order_offset_appendage_length
    );

    if (appendage_length.failed()) {
        return {appendage_length.error, message};
    }

    error = require_appendage_exact_length(
        bytes,
        length_replace_order_base,
        appendage_length.value
    );

    if (error != ErrorCode::ok) {
        return {error, message};
    }

    message.orig_user_ref_num = orig_user_ref_num.value;
    message.user_ref_num = user_ref_num.value;
    message.quantity = quantity.value;
    message.price = price.value;
    message.time_in_force = parsed_time_in_force.value;
    message.display = parsed_display.value;
    message.intermarket_sweep_eligibility =
        parsed_intermarket_sweep_eligibility.value;
    message.cl_ord_id = cl_ord_id.value;
    message.optional_appendage.assign(
        bytes.begin() + static_cast<std::ptrdiff_t>(replace_order_offset_optional_appendage),
        bytes.end()
    );

    if (!is_valid_replace_order_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

Result<CancelOrderMessage> decode_cancel_order_message(
    std::span<const std::byte> bytes
) {
    CancelOrderMessage message{};

    auto error = require_exact_message_type(bytes, MessageType::cancel_order);

    if (error != ErrorCode::ok) {
        return {error, message};
    }

    if (bytes.size() != length_cancel_order_without_appendage
        && bytes.size() < length_cancel_order_base) {
        return {ErrorCode::parse_error, message};
    }

    const auto user_ref_num = wire::read_u32_be(
        bytes,
        cancel_order_offset_user_ref_num
    );

    if (user_ref_num.failed()) {
        return {user_ref_num.error, message};
    }

    const auto quantity = wire::read_u32_be(
        bytes,
        cancel_order_offset_quantity
    );

    if (quantity.failed()) {
        return {quantity.error, message};
    }

    message.user_ref_num = user_ref_num.value;
    message.quantity = quantity.value;

    if (bytes.size() == length_cancel_order_without_appendage) {
        message.has_appendage_length = false;
    } else {
        const auto appendage_length = wire::read_u16_be(
            bytes,
            cancel_order_offset_appendage_length
        );

        if (appendage_length.failed()) {
            return {appendage_length.error, message};
        }

        error = require_appendage_exact_length(
            bytes,
            length_cancel_order_base,
            appendage_length.value
        );

        if (error != ErrorCode::ok) {
            return {error, message};
        }

        message.has_appendage_length = true;
        message.optional_appendage.assign(
            bytes.begin() + static_cast<std::ptrdiff_t>(cancel_order_offset_optional_appendage),
            bytes.end()
        );
    }

    if (!is_valid_cancel_order_message(message)) {
        return {ErrorCode::parse_error, message};
    }

    return {ErrorCode::ok, message};
}

} // namespace fgep::ouch
