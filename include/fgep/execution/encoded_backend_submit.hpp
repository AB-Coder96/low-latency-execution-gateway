#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/execution/execution_backend.hpp"
#include "fgep/ouch/ouch_wire_messages.hpp"

#include <cstdint>

namespace fgep::execution {

enum class EncodedSubmitDecision : std::uint8_t {
    accepted,
    rejected
};

enum class EncodedSubmitRejectSource : std::uint8_t {
    none,
    encode,
    backend
};

struct EncodedSubmitResult {
    EncodedSubmitDecision decision{EncodedSubmitDecision::rejected};
    EncodedSubmitRejectSource reject_source{EncodedSubmitRejectSource::none};

    ErrorCode encode_error{ErrorCode::ok};
    BackendSubmitResult backend_result{};

    [[nodiscard]] constexpr bool accepted() const noexcept {
        return decision == EncodedSubmitDecision::accepted;
    }

    [[nodiscard]] constexpr bool rejected() const noexcept {
        return decision == EncodedSubmitDecision::rejected;
    }
};

[[nodiscard]] EncodedSubmitResult submit_enter_order_to_backend(
    ExecutionBackend& backend,
    const ouch::EnterOrderMessage& message
);

[[nodiscard]] EncodedSubmitResult submit_replace_order_to_backend(
    ExecutionBackend& backend,
    const ouch::ReplaceOrderMessage& message
);

[[nodiscard]] EncodedSubmitResult submit_cancel_order_to_backend(
    ExecutionBackend& backend,
    const ouch::CancelOrderMessage& message
);

} // namespace fgep::execution