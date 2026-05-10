#include "fgep/ouch/ouch_encode.hpp"

#include "fgep/wire/byte_io.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <variant>

namespace fgep::ouch {
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

[[nodiscard]] ErrorCode require_exact_encode_length(
    std::span<std::byte> bytes,
    std::size_t expected_length
) noexcept {
    if (bytes.size() != expected_length) {
        return ErrorCode::parse_error;
    }

    return ErrorCode::ok;
}

[[nodiscard]] ErrorCode write_appendage(
    std::span<std::byte> bytes,
    std::size_t appendage_length_offset,
    std::size_t appendage_offset,
    const std::vector<std::byte>& optional_appendage
) noexcept {
    if (!appendage_length_fits(optional_appendage)) {
        return ErrorCode::invalid_argument;
    }

    auto error = wire::write_u16_be(
        bytes,
        appendage_length_offset,
        static_cast<AppendageLength>(optional_appendage.size())
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!wire::has_range(bytes, appendage_offset, optional_appendage.size())) {
        return ErrorCode::parse_error;
    }

    for (std::size_t index = 0; index < optional_appendage.size(); ++index) {
        bytes[appendage_offset + index] = optional_appendage[index];
    }

    return ErrorCode::ok;
}

} // namespace

ErrorCode encode_message_type(
    std::span<std::byte> bytes,
    MessageType message_type
) noexcept {
    return write_char(bytes, 0, to_char(message_type));
}

ErrorCode encode_message(
    std::span<std::byte> bytes,
    const Message& message
) noexcept {
    return std::visit(
        [&bytes](const auto& concrete_message) -> ErrorCode {
            using MessageT = std::decay_t<decltype(concrete_message)>;

            if constexpr (std::is_same_v<MessageT, EnterOrderMessage>) {
                return encode_enter_order_message(bytes, concrete_message);
            } else if constexpr (
                std::is_same_v<MessageT, ReplaceOrderMessage>
            ) {
                return encode_replace_order_message(bytes, concrete_message);
            } else if constexpr (
                std::is_same_v<MessageT, CancelOrderMessage>
            ) {
                return encode_cancel_order_message(bytes, concrete_message);
            } else {
                return ErrorCode::unsupported;
            }
        },
        message
    );
}

ErrorCode encode_enter_order_message(
    std::span<std::byte> bytes,
    const EnterOrderMessage& message
) noexcept {
    auto error = require_exact_encode_length(bytes, encoded_length(message));

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_enter_order_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = encode_message_type(bytes, MessageType::enter_order);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        enter_order_offset_user_ref_num,
        message.user_ref_num
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(bytes, enter_order_offset_side, to_char(message.side));

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        enter_order_offset_quantity,
        message.quantity
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_fixed_ascii<8>(
        bytes,
        enter_order_offset_symbol,
        message.symbol
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(bytes, enter_order_offset_price, message.price);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        enter_order_offset_time_in_force,
        to_char(message.time_in_force)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        enter_order_offset_display,
        to_char(message.display)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        enter_order_offset_capacity,
        to_char(message.capacity)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        enter_order_offset_intermarket_sweep_eligibility,
        to_char(message.intermarket_sweep_eligibility)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        enter_order_offset_cross_type,
        to_char(message.cross_type)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_fixed_ascii<14>(
        bytes,
        enter_order_offset_cl_ord_id,
        message.cl_ord_id
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return write_appendage(
        bytes,
        enter_order_offset_appendage_length,
        enter_order_offset_optional_appendage,
        message.optional_appendage
    );
}

ErrorCode encode_replace_order_message(
    std::span<std::byte> bytes,
    const ReplaceOrderMessage& message
) noexcept {
    auto error = require_exact_encode_length(bytes, encoded_length(message));

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_replace_order_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = encode_message_type(bytes, MessageType::replace_order);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        replace_order_offset_orig_user_ref_num,
        message.orig_user_ref_num
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        replace_order_offset_user_ref_num,
        message.user_ref_num
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        replace_order_offset_quantity,
        message.quantity
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(bytes, replace_order_offset_price, message.price);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        replace_order_offset_time_in_force,
        to_char(message.time_in_force)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        replace_order_offset_display,
        to_char(message.display)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = write_char(
        bytes,
        replace_order_offset_intermarket_sweep_eligibility,
        to_char(message.intermarket_sweep_eligibility)
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_fixed_ascii<14>(
        bytes,
        replace_order_offset_cl_ord_id,
        message.cl_ord_id
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    return write_appendage(
        bytes,
        replace_order_offset_appendage_length,
        replace_order_offset_optional_appendage,
        message.optional_appendage
    );
}

ErrorCode encode_cancel_order_message(
    std::span<std::byte> bytes,
    const CancelOrderMessage& message
) noexcept {
    auto error = require_exact_encode_length(bytes, encoded_length(message));

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!is_valid_cancel_order_message(message)) {
        return ErrorCode::invalid_argument;
    }

    error = encode_message_type(bytes, MessageType::cancel_order);

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        cancel_order_offset_user_ref_num,
        message.user_ref_num
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u32_be(
        bytes,
        cancel_order_offset_quantity,
        message.quantity
    );

    if (error != ErrorCode::ok) {
        return error;
    }

    if (!message.has_appendage_length) {
        return ErrorCode::ok;
    }

    return write_appendage(
        bytes,
        cancel_order_offset_appendage_length,
        cancel_order_offset_optional_appendage,
        message.optional_appendage
    );
}

} // namespace fgep::ouch
