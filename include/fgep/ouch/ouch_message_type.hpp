#pragma once

#include "fgep/core/errors.hpp"

#include <string_view>

namespace fgep::ouch {

enum class MessageType : char {
    enter_order = 'O',
    replace_order = 'U',
    cancel_order = 'X'
};

[[nodiscard]] constexpr char to_char(MessageType message_type) noexcept {
    return static_cast<char>(message_type);
}

[[nodiscard]] constexpr Result<MessageType> message_type_from_char(
    char value
) noexcept {
    switch (value) {
        case 'O':
            return {ErrorCode::ok, MessageType::enter_order};
        case 'U':
            return {ErrorCode::ok, MessageType::replace_order};
        case 'X':
            return {ErrorCode::ok, MessageType::cancel_order};
        default:
            return {ErrorCode::parse_error, MessageType::enter_order};
    }
}

[[nodiscard]] constexpr std::string_view to_string(
    MessageType message_type
) noexcept {
    switch (message_type) {
        case MessageType::enter_order:
            return "enter_order";
        case MessageType::replace_order:
            return "replace_order";
        case MessageType::cancel_order:
            return "cancel_order";
    }

    return "unknown";
}

} // namespace fgep::ouch