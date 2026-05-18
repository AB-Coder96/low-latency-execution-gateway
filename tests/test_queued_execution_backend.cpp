#include "fgep/execution/encoded_backend_submit.hpp"
#include "fgep/execution/queued_execution_backend.hpp"
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
    fgep::ouch::UserRefNum user_ref_num
) {
    return fgep::ouch::EnterOrderMessage{
        .user_ref_num = user_ref_num,
        .side = fgep::ouch::Side::buy,
        .quantity = 100U,
        .symbol = symbol("AAPL"),
        .price = 1'902'500U,
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

void test_encoded_submit_uses_spsc_queue() {
    using QueueBackend = fgep::execution::QueuedExecutionBackend<8U, 128U>;

    QueueBackend queue_backend{};
    fgep::execution::RecordingExecutionBackend recording_backend{};

    const auto result = fgep::execution::submit_enter_order_to_backend(
        queue_backend,
        enter(42U)
    );

    assert(result.accepted());
    assert(result.backend_result.accepted());
    assert(queue_backend.approximate_size() == 1U);
    assert(recording_backend.empty());

    const auto drained = queue_backend.drain_to(recording_backend, 8U);

    assert(drained == 1U);
    assert(queue_backend.empty());
    assert(recording_backend.submitted_count() == 1U);

    const auto& submission = recording_backend.submissions().front();

    assert(submission.user_ref_num == 42U);
    assert(submission.kind == fgep::execution::BackendOrderKind::enter);

    const auto decoded = fgep::ouch::decode_enter_order_message(
        std::span<const std::byte>{submission.payload}
    );

    assert(decoded.ok());
    assert(decoded.value.user_ref_num == 42U);
    assert(decoded.value.quantity == 100U);
    assert(decoded.value.symbol == symbol("AAPL"));
}

void test_queue_capacity_rejects_hot_path_overflow() {
    using QueueBackend = fgep::execution::QueuedExecutionBackend<2U, 128U>;

    QueueBackend queue_backend{};

    assert(
        fgep::execution::submit_enter_order_to_backend(
            queue_backend,
            enter(1U)
        ).accepted()
    );

    assert(
        fgep::execution::submit_enter_order_to_backend(
            queue_backend,
            enter(2U)
        ).accepted()
    );

    const auto full = fgep::execution::submit_enter_order_to_backend(
        queue_backend,
        enter(3U)
    );

    assert(full.rejected());
    assert(
        full.reject_source == fgep::execution::EncodedSubmitRejectSource::backend
    );
    assert(
        full.backend_result.reject_reason
        == fgep::execution::BackendRejectReason::capacity_exceeded
    );
}

} // namespace

int main() {
    test_encoded_submit_uses_spsc_queue();
    test_queue_capacity_rejects_hot_path_overflow();
    return 0;
}