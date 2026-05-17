#include "fgep/execution/encoded_backend_submit.hpp"
#include "fgep/ouch/ouch_decode.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cassert>
#include <span>

namespace {

[[nodiscard]] fgep::ouch::Symbol symbol(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<8>(text);
    assert(value.ok());
    return value.value;
}

[[nodiscard]] fgep::ouch::ClOrdId cl_ord_id(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<14>(text);
    assert(value.ok());
    return value.value;
}

[[nodiscard]] fgep::ouch::EnterOrderMessage enter(
    fgep::ouch::UserRefNum user_ref_num,
    fgep::ouch::Quantity quantity,
    fgep::ouch::Price4 price
) {
    return fgep::ouch::EnterOrderMessage{
        .user_ref_num = user_ref_num,
        .side = fgep::ouch::Side::buy,
        .quantity = quantity,
        .symbol = symbol("AAPL"),
        .price = price,
        .time_in_force = fgep::ouch::TimeInForce::day,
        .display = fgep::ouch::Display::visible,
        .capacity = fgep::ouch::Capacity::agency,
        .intermarket_sweep_eligibility =
            fgep::ouch::IntermarketSweepEligibility::not_eligible,
        .cross_type = fgep::ouch::CrossType::continuous_market,
        .cl_ord_id = cl_ord_id("ORDER1"),
        .optional_appendage = {}
    };
}

[[nodiscard]] fgep::ouch::ReplaceOrderMessage replace(
    fgep::ouch::UserRefNum original_user_ref_num,
    fgep::ouch::UserRefNum replacement_user_ref_num,
    fgep::ouch::Quantity quantity,
    fgep::ouch::Price4 price
) {
    return fgep::ouch::ReplaceOrderMessage{
        .orig_user_ref_num = original_user_ref_num,
        .user_ref_num = replacement_user_ref_num,
        .quantity = quantity,
        .price = price,
        .time_in_force = fgep::ouch::TimeInForce::day,
        .display = fgep::ouch::Display::visible,
        .intermarket_sweep_eligibility =
            fgep::ouch::IntermarketSweepEligibility::not_eligible,
        .cl_ord_id = cl_ord_id("REPLACE1"),
        .optional_appendage = {}
    };
}

[[nodiscard]] fgep::ouch::CancelOrderMessage cancel(
    fgep::ouch::UserRefNum user_ref_num,
    fgep::ouch::Quantity quantity
) {
    return fgep::ouch::CancelOrderMessage{
        .user_ref_num = user_ref_num,
        .quantity = quantity,
        .optional_appendage = {}
    };
}

} // namespace

int main() {
    using namespace fgep::execution;

    {
        RecordingExecutionBackend backend{};

        const auto result = submit_enter_order_to_backend(
            backend,
            enter(1, 100, 1'902'500)
        );

        assert(result.accepted());
        assert(result.reject_source == EncodedSubmitRejectSource::none);
        assert(result.encode_error == fgep::ErrorCode::ok);
        assert(result.backend_result.accepted());
        assert(result.backend_result.kind == BackendOrderKind::enter);
        assert(result.backend_result.user_ref_num == 1);
        assert(backend.submitted_count() == 1);

        const auto decoded = fgep::ouch::decode_enter_order_message(
            std::span<const std::byte>{backend.submissions()[0].payload}
        );

        assert(decoded.ok());
        assert(decoded.value.user_ref_num == 1);
        assert(decoded.value.quantity == 100);
        assert(decoded.value.price == 1'902'500);
        assert(decoded.value.symbol == symbol("AAPL"));
    }

    {
        RecordingExecutionBackend backend{};

        const auto result = submit_replace_order_to_backend(
            backend,
            replace(1, 2, 150, 1'903'000)
        );

        assert(result.accepted());
        assert(result.backend_result.kind == BackendOrderKind::replace);
        assert(result.backend_result.user_ref_num == 2);
        assert(backend.submitted_count() == 1);

        const auto decoded = fgep::ouch::decode_replace_order_message(
            std::span<const std::byte>{backend.submissions()[0].payload}
        );

        assert(decoded.ok());
        assert(decoded.value.orig_user_ref_num == 1);
        assert(decoded.value.user_ref_num == 2);
        assert(decoded.value.quantity == 150);
        assert(decoded.value.price == 1'903'000);
    }

    {
        RecordingExecutionBackend backend{};

        const auto result = submit_cancel_order_to_backend(
            backend,
            cancel(2, 0)
        );

        assert(result.accepted());
        assert(result.backend_result.kind == BackendOrderKind::cancel);
        assert(result.backend_result.user_ref_num == 2);
        assert(backend.submitted_count() == 1);

        const auto decoded = fgep::ouch::decode_cancel_order_message(
            std::span<const std::byte>{backend.submissions()[0].payload}
        );

        assert(decoded.ok());
        assert(decoded.value.user_ref_num == 2);
        assert(decoded.value.quantity == 0);
    }

    {
        RecordingExecutionBackend backend{};

        const auto result = submit_enter_order_to_backend(
            backend,
            enter(3, 0, 1'902'500)
        );

        assert(result.rejected());
        assert(result.reject_source == EncodedSubmitRejectSource::encode);
        assert(result.encode_error != fgep::ErrorCode::ok);
        assert(result.backend_result.rejected());
        assert(result.backend_result.user_ref_num == 3);
        assert(result.backend_result.kind == BackendOrderKind::enter);
        assert(backend.empty());
    }

    {
        RecordingExecutionBackend backend{};
        backend.set_open(false);

        const auto result = submit_enter_order_to_backend(
            backend,
            enter(4, 100, 1'902'500)
        );

        assert(result.rejected());
        assert(result.reject_source == EncodedSubmitRejectSource::backend);
        assert(result.encode_error == fgep::ErrorCode::ok);
        assert(result.backend_result.rejected());
        assert(
            result.backend_result.reject_reason
            == BackendRejectReason::backend_closed
        );
        assert(backend.empty());
    }

    return 0;
}