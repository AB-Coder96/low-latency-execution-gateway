#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/ouch/ouch_wire_messages.hpp"

#include <cstddef>
#include <span>

namespace fgep::ouch {

[[nodiscard]] Result<MessageType> decode_message_type(
    std::span<const std::byte> bytes
) noexcept;

[[nodiscard]] ErrorCode require_exact_message_type(
    std::span<const std::byte> bytes,
    MessageType expected_message_type
) noexcept;

[[nodiscard]] Result<Message> decode_message(
    std::span<const std::byte> bytes
);

[[nodiscard]] Result<EnterOrderMessage> decode_enter_order_message(
    std::span<const std::byte> bytes
);

[[nodiscard]] Result<ReplaceOrderMessage> decode_replace_order_message(
    std::span<const std::byte> bytes
);

[[nodiscard]] Result<CancelOrderMessage> decode_cancel_order_message(
    std::span<const std::byte> bytes
);

} // namespace fgep::ouch