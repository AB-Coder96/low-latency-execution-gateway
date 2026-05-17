#include "fgep/execution/encoded_backend_submit.hpp"

#include "fgep/ouch/ouch_encode.hpp"

#include <cstddef>
#include <vector>

namespace fgep::execution {
namespace {

[[nodiscard]] EncodedSubmitResult make_encode_reject(
    ErrorCode error,
    ouch::UserRefNum user_ref_num,
    BackendOrderKind kind
) noexcept {
    return EncodedSubmitResult{
        .decision = EncodedSubmitDecision::rejected,
        .reject_source = EncodedSubmitRejectSource::encode,
        .encode_error = error,
        .backend_result = BackendSubmitResult{
            .decision = BackendSubmitDecision::rejected,
            .reject_reason = BackendRejectReason::none,
            .user_ref_num = user_ref_num,
            .kind = kind,
            .payload_size = 0
        }
    };
}

[[nodiscard]] EncodedSubmitResult make_backend_result(
    const BackendSubmitResult& backend_result
) noexcept {
    return EncodedSubmitResult{
        .decision = backend_result.accepted()
            ? EncodedSubmitDecision::accepted
            : EncodedSubmitDecision::rejected,
        .reject_source = backend_result.accepted()
            ? EncodedSubmitRejectSource::none
            : EncodedSubmitRejectSource::backend,
        .encode_error = ErrorCode::ok,
        .backend_result = backend_result
    };
}

[[nodiscard]] EncodedSubmitResult submit_encoded_payload(
    ExecutionBackend& backend,
    ouch::UserRefNum user_ref_num,
    BackendOrderKind kind,
    std::vector<std::byte>& payload
) {
    return make_backend_result(
        backend.submit(BackendSubmitRequest{
            .user_ref_num = user_ref_num,
            .kind = kind,
            .payload = payload
        })
    );
}

} // namespace

EncodedSubmitResult submit_enter_order_to_backend(
    ExecutionBackend& backend,
    const ouch::EnterOrderMessage& message
) {
    std::vector<std::byte> payload(ouch::encoded_length(message));

    const auto error = ouch::encode_enter_order_message(payload, message);

    if (error != ErrorCode::ok) {
        return make_encode_reject(
            error,
            message.user_ref_num,
            BackendOrderKind::enter
        );
    }

    return submit_encoded_payload(
        backend,
        message.user_ref_num,
        BackendOrderKind::enter,
        payload
    );
}

EncodedSubmitResult submit_replace_order_to_backend(
    ExecutionBackend& backend,
    const ouch::ReplaceOrderMessage& message
) {
    std::vector<std::byte> payload(ouch::encoded_length(message));

    const auto error = ouch::encode_replace_order_message(payload, message);

    if (error != ErrorCode::ok) {
        return make_encode_reject(
            error,
            message.user_ref_num,
            BackendOrderKind::replace
        );
    }

    return submit_encoded_payload(
        backend,
        message.user_ref_num,
        BackendOrderKind::replace,
        payload
    );
}

EncodedSubmitResult submit_cancel_order_to_backend(
    ExecutionBackend& backend,
    const ouch::CancelOrderMessage& message
) {
    std::vector<std::byte> payload(ouch::encoded_length(message));

    const auto error = ouch::encode_cancel_order_message(payload, message);

    if (error != ErrorCode::ok) {
        return make_encode_reject(
            error,
            message.user_ref_num,
            BackendOrderKind::cancel
        );
    }

    return submit_encoded_payload(
        backend,
        message.user_ref_num,
        BackendOrderKind::cancel,
        payload
    );
}

} // namespace fgep::execution