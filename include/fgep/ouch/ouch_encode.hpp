#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/ouch/ouch_wire_messages.hpp"

#include <cstddef>
#include <span>

namespace fgep::ouch {

[[nodiscard]] ErrorCode encode_message_type(
    std::span<std::byte> bytes,
    MessageType message_type
) noexcept;

[[nodiscard]] ErrorCode encode_message(
    std::span<std::byte> bytes,
    const Message& message
) noexcept;

[[nodiscard]] ErrorCode encode_enter_order_message(
    std::span<std::byte> bytes,
    const EnterOrderMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_replace_order_message(
    std::span<std::byte> bytes,
    const ReplaceOrderMessage& message
) noexcept;

[[nodiscard]] ErrorCode encode_cancel_order_message(
    std::span<std::byte> bytes,
    const CancelOrderMessage& message
) noexcept;

} // namespace fgep::ouch