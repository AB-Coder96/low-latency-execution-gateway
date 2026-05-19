#pragma once

#include "fgep/core/spsc_ring.hpp"
#include "fgep/execution/execution_backend.hpp"
#include "fgep/telemetry/core_stats.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace fgep::execution {

inline constexpr std::size_t max_backend_submit_payload_size = 256U;

inline constexpr std::size_t default_backend_submit_payload_bytes =
    max_backend_submit_payload_size;

enum class BackendSubmitQueuePushStatus : std::uint8_t {
    queued,
    empty_payload,
    payload_too_large,
    queue_full
};

struct BackendSubmitQueuePushResult {
    BackendSubmitQueuePushStatus status{
        BackendSubmitQueuePushStatus::queue_full
    };

    [[nodiscard]] constexpr bool queued() const noexcept {
        return status == BackendSubmitQueuePushStatus::queued;
    }
};

template <std::size_t MaxPayloadSize = max_backend_submit_payload_size>
struct BackendSubmitQueueItem {
    static_assert(MaxPayloadSize > 0U, "MaxPayloadSize must be positive");
    static_assert(
        MaxPayloadSize <= static_cast<std::size_t>(
            std::numeric_limits<std::uint16_t>::max()
        ),
        "MaxPayloadSize must fit in uint16_t"
    );

    ouch::UserRefNum user_ref_num{};
    BackendOrderKind kind{BackendOrderKind::enter};
    std::uint16_t payload_size{};
    std::array<std::byte, MaxPayloadSize> payload{};

    [[nodiscard]] std::span<const std::byte> payload_span() const noexcept {
        return std::span<const std::byte>{
            payload.data(),
            static_cast<std::size_t>(payload_size)
        };
    }

    [[nodiscard]] BackendSubmitRequest request() const noexcept {
        return BackendSubmitRequest{
            .user_ref_num = user_ref_num,
            .kind = kind,
            .payload = payload_span()
        };
    }

    [[nodiscard]] BackendSubmitRequest as_request() const noexcept {
        return request();
    }
};

template <
    std::size_t Capacity,
    std::size_t MaxPayloadSize = max_backend_submit_payload_size
>
class BackendSubmitQueue {
public:
    using Item = BackendSubmitQueueItem<MaxPayloadSize>;
    using Entry = Item;

    void set_stats(::fgep::CoreStats* stats) noexcept {
        stats_ = stats;
    }

    [[nodiscard]] BackendSubmitQueuePushResult push(
        const BackendSubmitRequest& request
    ) noexcept {
        Item item{};

        const auto status = make_item(request, item);

        if (status != BackendSubmitQueuePushStatus::queued) {
            record_rejected();
            return BackendSubmitQueuePushResult{.status = status};
        }

        if (!ring_.push(item)) {
            record_ring_full();
            record_rejected();

            return BackendSubmitQueuePushResult{
                .status = BackendSubmitQueuePushStatus::queue_full
            };
        }

        record_queued();

        return BackendSubmitQueuePushResult{
            .status = BackendSubmitQueuePushStatus::queued
        };
    }

    [[nodiscard]] BackendSubmitResult try_push(
        const BackendSubmitRequest& request
    ) noexcept {
        const auto result = push(request);

        if (result.queued()) {
            return make_backend_accept(request);
        }

        return make_backend_reject(
            request,
            reject_reason_for(result.status)
        );
    }

    [[nodiscard]] bool pop(Item& item) noexcept {
        return ring_.pop(item);
    }

    [[nodiscard]] bool try_pop(Item& item) noexcept {
        return pop(item);
    }

    [[nodiscard]] bool empty() const noexcept {
        return ring_.empty();
    }

    [[nodiscard]] std::size_t approximate_size() const noexcept {
        return ring_.approximate_size();
    }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return Capacity;
    }

    [[nodiscard]] static constexpr std::size_t max_payload_size() noexcept {
        return MaxPayloadSize;
    }

private:
    [[nodiscard]] static BackendRejectReason reject_reason_for(
        BackendSubmitQueuePushStatus status
    ) noexcept {
        switch (status) {
            case BackendSubmitQueuePushStatus::empty_payload:
                return BackendRejectReason::empty_payload;

            case BackendSubmitQueuePushStatus::payload_too_large:
            case BackendSubmitQueuePushStatus::queue_full:
                return BackendRejectReason::capacity_exceeded;

            case BackendSubmitQueuePushStatus::queued:
                return BackendRejectReason::none;
        }

        return BackendRejectReason::capacity_exceeded;
    }

    void record_queued() noexcept {
        if (stats_ != nullptr) {
            stats_->increment(::fgep::CoreCounter::submit_queued);
        }
    }

    void record_rejected() noexcept {
        if (stats_ != nullptr) {
            stats_->increment(::fgep::CoreCounter::submit_rejected);
        }
    }

    void record_ring_full() noexcept {
        if (stats_ != nullptr) {
            stats_->increment(::fgep::CoreCounter::ring_full);
        }
    }

    [[nodiscard]] static BackendSubmitQueuePushStatus make_item(
        const BackendSubmitRequest& request,
        Item& item
    ) noexcept {
        if (request.payload.empty()) {
            return BackendSubmitQueuePushStatus::empty_payload;
        }

        if (request.payload.size() > MaxPayloadSize) {
            return BackendSubmitQueuePushStatus::payload_too_large;
        }

        item.user_ref_num = request.user_ref_num;
        item.kind = request.kind;
        item.payload_size = static_cast<std::uint16_t>(
            request.payload.size()
        );

        std::copy(
            request.payload.begin(),
            request.payload.end(),
            item.payload.begin()
        );

        return BackendSubmitQueuePushStatus::queued;
    }

    ::fgep::SpscRing<Item, Capacity> ring_{};
    ::fgep::CoreStats* stats_{nullptr};
};

static_assert(alignof(BackendSubmitQueue<1024U>) == ::fgep::cache_line_size);

} // namespace fgep::execution