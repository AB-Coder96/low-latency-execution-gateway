#include "fgep/execution/async_execution_backend.hpp"
#include "fgep/execution/execution_backend.hpp"
#include "fgep/telemetry/core_stats.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>

namespace {

using AsyncBackend = fgep::execution::AsyncExecutionBackend<4U, 8U>;

[[nodiscard]] std::array<std::byte, 3U> payload3(
    std::uint8_t first,
    std::uint8_t second,
    std::uint8_t third
) noexcept {
    return std::array<std::byte, 3U>{
        std::byte{first},
        std::byte{second},
        std::byte{third}
    };
}

[[nodiscard]] fgep::execution::BackendSubmitRequest request_from(
    fgep::ouch::UserRefNum user_ref_num,
    fgep::execution::BackendOrderKind kind,
    std::span<const std::byte> payload
) noexcept {
    return fgep::execution::BackendSubmitRequest{
        .user_ref_num = user_ref_num,
        .kind = kind,
        .payload = payload
    };
}

void test_manual_poll_submits_owned_payload() {
    fgep::execution::RecordingExecutionBackend backend{};
    fgep::CoreStats stats{};

    AsyncBackend async{
        backend,
        &stats,
        false
    };

    auto payload = payload3(0x01U, 0x02U, 0x03U);

    const auto submit_result = async.submit(request_from(
        11U,
        fgep::execution::BackendOrderKind::enter,
        payload
    ));

    assert(submit_result.accepted());
    assert(backend.empty());
    assert(stats.load(fgep::CoreCounter::submit_queued) == 1U);

    payload[0] = std::byte{0xffU};
    payload[1] = std::byte{0xeeU};
    payload[2] = std::byte{0xddU};

    assert(async.poll_once());
    assert(!async.poll_once());

    assert(backend.submitted_count() == 1U);
    assert(backend.submissions()[0U].user_ref_num == 11U);
    assert(
        backend.submissions()[0U].kind
        == fgep::execution::BackendOrderKind::enter
    );
    assert(backend.submissions()[0U].payload.size() == 3U);
    assert(backend.submissions()[0U].payload[0U] == std::byte{0x01U});
    assert(backend.submissions()[0U].payload[1U] == std::byte{0x02U});
    assert(backend.submissions()[0U].payload[2U] == std::byte{0x03U});

    assert(stats.load(fgep::CoreCounter::backend_accepted) == 1U);
    assert(stats.load(fgep::CoreCounter::backend_rejected) == 0U);
}

void test_rejects_invalid_or_full_queue() {
    fgep::execution::RecordingExecutionBackend backend{};
    fgep::CoreStats stats{};

    AsyncBackend async{
        backend,
        &stats,
        false
    };

    const std::array<std::byte, 0U> empty{};

    const auto empty_result = async.submit(request_from(
        1U,
        fgep::execution::BackendOrderKind::enter,
        empty
    ));

    assert(empty_result.rejected());
    assert(
        empty_result.reject_reason
        == fgep::execution::BackendRejectReason::empty_payload
    );

    const std::array<std::byte, 9U> oversized{};

    const auto oversized_result = async.submit(request_from(
        2U,
        fgep::execution::BackendOrderKind::replace,
        oversized
    ));

    assert(oversized_result.rejected());
    assert(
        oversized_result.reject_reason
        == fgep::execution::BackendRejectReason::capacity_exceeded
    );

    const auto one = payload3(1U, 1U, 1U);
    const auto two = payload3(2U, 2U, 2U);
    const auto three = payload3(3U, 3U, 3U);
    const auto four = payload3(4U, 4U, 4U);
    const auto five = payload3(5U, 5U, 5U);

    assert(async.submit(request_from(
        3U,
        fgep::execution::BackendOrderKind::enter,
        one
    )).accepted());

    assert(async.submit(request_from(
        4U,
        fgep::execution::BackendOrderKind::enter,
        two
    )).accepted());

    assert(async.submit(request_from(
        5U,
        fgep::execution::BackendOrderKind::enter,
        three
    )).accepted());

    assert(async.submit(request_from(
        6U,
        fgep::execution::BackendOrderKind::enter,
        four
    )).accepted());

    const auto full_result = async.submit(request_from(
        7U,
        fgep::execution::BackendOrderKind::enter,
        five
    ));

    assert(full_result.rejected());
    assert(
        full_result.reject_reason
        == fgep::execution::BackendRejectReason::capacity_exceeded
    );

    assert(stats.load(fgep::CoreCounter::submit_queued) == 4U);
    assert(stats.load(fgep::CoreCounter::submit_rejected) == 3U);
    assert(stats.load(fgep::CoreCounter::ring_full) == 1U);
}

void test_manual_drain() {
    fgep::execution::RecordingExecutionBackend backend{};
    fgep::CoreStats stats{};

    AsyncBackend async{
        backend,
        &stats,
        false
    };

    const auto payload = payload3(0xaaU, 0xbbU, 0xccU);

    assert(async.submit(request_from(
        21U,
        fgep::execution::BackendOrderKind::enter,
        payload
    )).accepted());

    assert(async.submit(request_from(
        22U,
        fgep::execution::BackendOrderKind::cancel,
        payload
    )).accepted());

    assert(async.approximate_size() == 2U);
    assert(async.drain() == 2U);
    assert(async.empty());

    assert(backend.submitted_count() == 2U);
    assert(backend.submissions()[0U].user_ref_num == 21U);
    assert(backend.submissions()[1U].user_ref_num == 22U);

    assert(stats.load(fgep::CoreCounter::backend_accepted) == 2U);
}

void test_worker_drains_on_stop() {
    fgep::execution::RecordingExecutionBackend backend{};
    fgep::CoreStats stats{};

    {
        fgep::execution::AsyncExecutionBackend<1024U, 16U> async{
            backend,
            &stats,
            true
        };

        constexpr std::uint32_t message_count = 1000U;

        for (std::uint32_t sequence = 1U;
             sequence <= message_count;
             ++sequence) {
            const std::array<std::byte, 4U> payload{
                std::byte{static_cast<std::uint8_t>(sequence & 0xffU)},
                std::byte{0x12U},
                std::byte{0x34U},
                std::byte{0x56U}
            };

            const auto result = async.submit(request_from(
                sequence,
                fgep::execution::BackendOrderKind::enter,
                payload
            ));

            assert(result.accepted());
        }

        async.stop();
    }

    assert(backend.submitted_count() == 1000U);
    assert(stats.load(fgep::CoreCounter::submit_queued) == 1000U);
    assert(stats.load(fgep::CoreCounter::backend_accepted) == 1000U);
    assert(stats.load(fgep::CoreCounter::backend_rejected) == 0U);
}

void test_backend_rejection_counter() {
    fgep::execution::RecordingExecutionBackend backend{1U};
    fgep::CoreStats stats{};

    AsyncBackend async{
        backend,
        &stats,
        false
    };

    const auto payload = payload3(1U, 2U, 3U);

    assert(async.submit(request_from(
        31U,
        fgep::execution::BackendOrderKind::enter,
        payload
    )).accepted());

    assert(async.submit(request_from(
        32U,
        fgep::execution::BackendOrderKind::enter,
        payload
    )).accepted());

    assert(async.drain() == 2U);

    assert(backend.submitted_count() == 1U);
    assert(stats.load(fgep::CoreCounter::backend_accepted) == 1U);
    assert(stats.load(fgep::CoreCounter::backend_rejected) == 1U);
}

} // namespace

int main() {
    test_manual_poll_submits_owned_payload();
    test_rejects_invalid_or_full_queue();
    test_manual_drain();
    test_worker_drains_on_stop();
    test_backend_rejection_counter();

    std::cout << "Async SPSC execution backend test passed\n";
    return 0;
}