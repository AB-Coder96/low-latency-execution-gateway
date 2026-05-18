#include "fgep/execution/backend_submit_queue.hpp"

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <thread>

namespace {

[[nodiscard]] std::array<std::byte, 4U> small_payload() noexcept {
    return {
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04}
    };
}

void test_submit_payload_is_copied() {
    using Queue = fgep::execution::BackendSubmitQueue<4U, 8U>;
    using Entry = Queue::Entry;

    Queue queue{};
    auto bytes = small_payload();

    const auto result = queue.try_push(fgep::execution::BackendSubmitRequest{
        .user_ref_num = 100U,
        .kind = fgep::execution::BackendOrderKind::enter,
        .payload = bytes
    });

    assert(result.accepted());
    assert(result.payload_size == bytes.size());
    assert(queue.approximate_size() == 1U);

    bytes[0] = std::byte{0xff};

    Entry entry{};
    assert(queue.try_pop(entry));
    assert(entry.user_ref_num == 100U);
    assert(entry.kind == fgep::execution::BackendOrderKind::enter);
    assert(entry.payload_size == 4U);
    assert(entry.payload[0] == std::byte{0x01});
    assert(entry.payload[3] == std::byte{0x04});

    const auto request = entry.as_request();
    assert(request.user_ref_num == 100U);
    assert(request.kind == fgep::execution::BackendOrderKind::enter);
    assert(request.payload.size() == 4U);
    assert(request.payload[0] == std::byte{0x01});
    assert(queue.empty());
}

void test_empty_too_large_and_full_rejections() {
    fgep::execution::BackendSubmitQueue<2U, 4U> queue{};

    const auto empty = queue.try_push(fgep::execution::BackendSubmitRequest{
        .user_ref_num = 1U,
        .kind = fgep::execution::BackendOrderKind::cancel,
        .payload = {}
    });

    assert(empty.rejected());
    assert(
        empty.reject_reason == fgep::execution::BackendRejectReason::empty_payload
    );

    const std::array<std::byte, 5U> too_large{};
    const auto oversized = queue.try_push(
        fgep::execution::BackendSubmitRequest{
            .user_ref_num = 2U,
            .kind = fgep::execution::BackendOrderKind::replace,
            .payload = too_large
        }
    );

    assert(oversized.rejected());
    assert(
        oversized.reject_reason
        == fgep::execution::BackendRejectReason::capacity_exceeded
    );

    const auto bytes = small_payload();

    assert(
        queue.try_push(fgep::execution::BackendSubmitRequest{
            .user_ref_num = 3U,
            .kind = fgep::execution::BackendOrderKind::enter,
            .payload = bytes
        }).accepted()
    );
    assert(
        queue.try_push(fgep::execution::BackendSubmitRequest{
            .user_ref_num = 4U,
            .kind = fgep::execution::BackendOrderKind::enter,
            .payload = bytes
        }).accepted()
    );

    const auto full = queue.try_push(fgep::execution::BackendSubmitRequest{
        .user_ref_num = 5U,
        .kind = fgep::execution::BackendOrderKind::enter,
        .payload = bytes
    });

    assert(full.rejected());
    assert(
        full.reject_reason == fgep::execution::BackendRejectReason::capacity_exceeded
    );
}

void test_acquire_release_handoff() {
    using Queue = fgep::execution::BackendSubmitQueue<1024U, 16U>;
    using Entry = Queue::Entry;

    Queue queue{};
    constexpr std::uint32_t count = 1'000'000U;

    std::atomic<bool> start{false};

    std::thread producer{[&queue, &start]() {
        while (!start.load(std::memory_order_acquire)) {
        }

        for (std::uint32_t i = 1U; i <= count; ++i) {
            const std::array<std::byte, 4U> payload{
                std::byte{static_cast<unsigned char>(i & 0xffU)},
                std::byte{static_cast<unsigned char>((i >> 8U) & 0xffU)},
                std::byte{static_cast<unsigned char>((i >> 16U) & 0xffU)},
                std::byte{static_cast<unsigned char>((i >> 24U) & 0xffU)}
            };

            const fgep::execution::BackendSubmitRequest request{
                .user_ref_num = i,
                .kind = fgep::execution::BackendOrderKind::enter,
                .payload = payload
            };

            while (!queue.try_push(request).accepted()) {
            }
        }
    }};

    std::thread consumer{[&queue, &start]() {
        while (!start.load(std::memory_order_acquire)) {
        }

        for (std::uint32_t expected = 1U; expected <= count; ++expected) {
            Entry entry{};

            while (!queue.try_pop(entry)) {
            }

            assert(entry.user_ref_num == expected);
            assert(entry.kind == fgep::execution::BackendOrderKind::enter);
            assert(entry.payload_size == 4U);
            assert(
                entry.payload[0]
                == std::byte{static_cast<unsigned char>(expected & 0xffU)}
            );
        }
    }};

    start.store(true, std::memory_order_release);

    producer.join();
    consumer.join();

    assert(queue.empty());
}

} // namespace

int main() {
    test_submit_payload_is_copied();
    test_empty_too_large_and_full_rejections();
    test_acquire_release_handoff();
    return 0;
}