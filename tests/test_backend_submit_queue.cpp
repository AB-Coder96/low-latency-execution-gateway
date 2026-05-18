#include "fgep/execution/backend_submit_queue.hpp"
#include "fgep/telemetry/core_stats.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <thread>

namespace {

using Queue = fgep::execution::BackendSubmitQueue<4U, 8U>;
using Item = Queue::Item;

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

void test_rejects_empty_and_oversized_payloads() {
    Queue queue{};

    const std::array<std::byte, 0U> empty{};

    const auto empty_result = queue.push(request_from(
        1U,
        fgep::execution::BackendOrderKind::enter,
        empty
    ));

    assert(!empty_result.queued());
    assert(
        empty_result.status
        == fgep::execution::BackendSubmitQueuePushStatus::empty_payload
    );

    const std::array<std::byte, 9U> oversized{};

    const auto oversized_result = queue.push(request_from(
        2U,
        fgep::execution::BackendOrderKind::replace,
        oversized
    ));

    assert(!oversized_result.queued());
    assert(
        oversized_result.status
        == fgep::execution::BackendSubmitQueuePushStatus::payload_too_large
    );

    assert(queue.empty());
}

void test_queue_owns_payload_bytes() {
    Queue queue{};
    auto payload = payload3(0x01U, 0x02U, 0x03U);

    const auto result = queue.push(request_from(
        7U,
        fgep::execution::BackendOrderKind::cancel,
        payload
    ));

    assert(result.queued());

    payload[0] = std::byte{0xffU};
    payload[1] = std::byte{0xeeU};
    payload[2] = std::byte{0xddU};

    Item item{};
    assert(queue.pop(item));
    assert(item.user_ref_num == 7U);
    assert(item.kind == fgep::execution::BackendOrderKind::cancel);
    assert(item.payload_size == 3U);

    const auto view = item.payload_span();
    assert(view.size() == 3U);
    assert(view[0] == std::byte{0x01U});
    assert(view[1] == std::byte{0x02U});
    assert(view[2] == std::byte{0x03U});

    const auto backend_request = item.request();
    assert(backend_request.user_ref_num == item.user_ref_num);
    assert(backend_request.kind == item.kind);
    assert(backend_request.payload.size() == view.size());
    assert(backend_request.payload[0] == view[0]);
    assert(backend_request.payload[1] == view[1]);
    assert(backend_request.payload[2] == view[2]);
}

void test_full_empty_and_wraparound() {
    Queue queue{};

    const auto one = payload3(1U, 1U, 1U);
    const auto two = payload3(2U, 2U, 2U);
    const auto three = payload3(3U, 3U, 3U);
    const auto four = payload3(4U, 4U, 4U);
    const auto five = payload3(5U, 5U, 5U);

    assert(queue.push(request_from(
        1U,
        fgep::execution::BackendOrderKind::enter,
        one
    )).queued());

    assert(queue.push(request_from(
        2U,
        fgep::execution::BackendOrderKind::enter,
        two
    )).queued());

    assert(queue.push(request_from(
        3U,
        fgep::execution::BackendOrderKind::enter,
        three
    )).queued());

    assert(queue.push(request_from(
        4U,
        fgep::execution::BackendOrderKind::enter,
        four
    )).queued());

    const auto full_result = queue.push(request_from(
        5U,
        fgep::execution::BackendOrderKind::enter,
        five
    ));

    assert(!full_result.queued());
    assert(
        full_result.status
        == fgep::execution::BackendSubmitQueuePushStatus::queue_full
    );

    Item item{};

    assert(queue.pop(item));
    assert(item.user_ref_num == 1U);

    assert(queue.pop(item));
    assert(item.user_ref_num == 2U);

    assert(queue.push(request_from(
        5U,
        fgep::execution::BackendOrderKind::replace,
        five
    )).queued());

    assert(queue.push(request_from(
        6U,
        fgep::execution::BackendOrderKind::cancel,
        one
    )).queued());

    assert(queue.pop(item));
    assert(item.user_ref_num == 3U);

    assert(queue.pop(item));
    assert(item.user_ref_num == 4U);

    assert(queue.pop(item));
    assert(item.user_ref_num == 5U);
    assert(item.kind == fgep::execution::BackendOrderKind::replace);

    assert(queue.pop(item));
    assert(item.user_ref_num == 6U);
    assert(item.kind == fgep::execution::BackendOrderKind::cancel);

    assert(!queue.pop(item));
    assert(queue.empty());
}

void test_spsc_handoff() {
    fgep::execution::BackendSubmitQueue<1024U, 64U> queue{};

    constexpr std::uint32_t message_count = 100'000U;

    std::thread producer{[&queue]() {
        for (std::uint32_t sequence = 1U;
             sequence <= message_count;
             ++sequence) {
            const std::array<std::byte, 4U> payload{
                std::byte{static_cast<std::uint8_t>(sequence & 0xffU)},
                std::byte{0x12U},
                std::byte{0x34U},
                std::byte{0x56U}
            };

            const auto request = request_from(
                sequence,
                fgep::execution::BackendOrderKind::enter,
                payload
            );

            while (!queue.push(request).queued()) {
            }
        }
    }};

    std::thread consumer{[&queue]() {
        for (std::uint32_t expected = 1U;
             expected <= message_count;
             ++expected) {
            fgep::execution::BackendSubmitQueue<1024U, 64U>::Item item{};

            while (!queue.pop(item)) {
            }

            assert(item.user_ref_num == expected);
            assert(item.kind == fgep::execution::BackendOrderKind::enter);
            assert(item.payload_size == 4U);
            assert(
                item.payload[0]
                == std::byte{static_cast<std::uint8_t>(expected & 0xffU)}
            );
        }
    }};

    producer.join();
    consumer.join();

    assert(queue.empty());
}

void test_queue_updates_core_stats() {
    Queue queue{};
    fgep::CoreStats stats{};

    queue.set_stats(&stats);

    const auto payload = payload3(1U, 2U, 3U);

    assert(queue.push(request_from(
        1U,
        fgep::execution::BackendOrderKind::enter,
        payload
    )).queued());

    assert(stats.load(fgep::CoreCounter::submit_queued) == 1U);
    assert(stats.load(fgep::CoreCounter::submit_rejected) == 0U);
    assert(stats.load(fgep::CoreCounter::ring_full) == 0U);

    const std::array<std::byte, 0U> empty{};

    const auto empty_result = queue.push(request_from(
        2U,
        fgep::execution::BackendOrderKind::cancel,
        empty
    ));

    assert(!empty_result.queued());
    assert(stats.load(fgep::CoreCounter::submit_queued) == 1U);
    assert(stats.load(fgep::CoreCounter::submit_rejected) == 1U);
    assert(stats.load(fgep::CoreCounter::ring_full) == 0U);

    const auto two = payload3(2U, 2U, 2U);
    const auto three = payload3(3U, 3U, 3U);
    const auto four = payload3(4U, 4U, 4U);
    const auto five = payload3(5U, 5U, 5U);

    assert(queue.push(request_from(
        3U,
        fgep::execution::BackendOrderKind::enter,
        two
    )).queued());

    assert(queue.push(request_from(
        4U,
        fgep::execution::BackendOrderKind::enter,
        three
    )).queued());

    assert(queue.push(request_from(
        5U,
        fgep::execution::BackendOrderKind::enter,
        four
    )).queued());

    const auto full_result = queue.push(request_from(
        6U,
        fgep::execution::BackendOrderKind::enter,
        five
    ));

    assert(!full_result.queued());
    assert(
        full_result.status
        == fgep::execution::BackendSubmitQueuePushStatus::queue_full
    );

    assert(stats.load(fgep::CoreCounter::submit_queued) == 4U);
    assert(stats.load(fgep::CoreCounter::submit_rejected) == 2U);
    assert(stats.load(fgep::CoreCounter::ring_full) == 1U);
}

} // namespace

int main() {
    test_rejects_empty_and_oversized_payloads();
    test_queue_owns_payload_bytes();
    test_full_empty_and_wraparound();
    test_spsc_handoff();
    test_queue_updates_core_stats();

    std::cout << "Backend submit SPSC queue test passed\n";
    return 0;
}