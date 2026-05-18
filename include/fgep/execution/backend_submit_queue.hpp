#pragma once

#include "fgep/core/spsc_ring.hpp"
#include "fgep/execution/execution_backend.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace fgep::execution {

inline constexpr std::size_t default_backend_submit_payload_bytes = 256U;

template <std::size_t MaxPayloadBytes>
struct QueuedBackendSubmit {
    static_assert(MaxPayloadBytes > 0U, "MaxPayloadBytes must be non-zero");
    static_assert(
        MaxPayloadBytes <= std::numeric_limits<std::uint16_t>::max(),
        "QueuedBackendSubmit stores payload size as uint16_t"
    );

    ouch::UserRefNum user_ref_num{};
    BackendOrderKind kind{BackendOrderKind::enter};
    std::uint16_t payload_size{};
    std::array<std::byte, MaxPayloadBytes> payload{};

    [[nodiscard]] std::span<const std::byte> payload_view() const noexcept {
        return {payload.data(), static_cast<std::size_t>(payload_size)};
    }

    [[nodiscard]] BackendSubmitRequest as_request() const noexcept {
        return BackendSubmitRequest{
            .user_ref_num = user_ref_num,
            .kind = kind,
            .payload = payload_view()
        };
    }
};

template <
    std::size_t QueueCapacity,
    std::size_t MaxPayloadBytes = default_backend_submit_payload_bytes
>
class BackendSubmitQueue {
public:
    using Entry = QueuedBackendSubmit<MaxPayloadBytes>;

    BackendSubmitQueue() = default;

    BackendSubmitQueue(const BackendSubmitQueue&) = delete;
    BackendSubmitQueue& operator=(const BackendSubmitQueue&) = delete;

    BackendSubmitQueue(BackendSubmitQueue&&) = delete;
    BackendSubmitQueue& operator=(BackendSubmitQueue&&) = delete;

    [[nodiscard]] BackendSubmitResult try_push(
        const BackendSubmitRequest& request
    ) noexcept {
        if (request.payload.empty()) {
            return make_backend_reject(
                request,
                BackendRejectReason::empty_payload
            );
        }

        if (request.payload.size() > MaxPayloadBytes) {
            return make_backend_reject(
                request,
                BackendRejectReason::capacity_exceeded
            );
        }

        Entry entry{
            .user_ref_num = request.user_ref_num,
            .kind = request.kind,
            .payload_size = static_cast<std::uint16_t>(request.payload.size()),
            .payload = {}
        };

        std::copy(
            request.payload.begin(),
            request.payload.end(),
            entry.payload.begin()
        );

        if (!ring_.push(entry)) {
            return make_backend_reject(
                request,
                BackendRejectReason::capacity_exceeded
            );
        }

        return make_backend_accept(request);
    }

    [[nodiscard]] bool try_pop(Entry& out) noexcept {
        return ring_.pop(out);
    }

    [[nodiscard]] bool empty() const noexcept {
        return ring_.empty();
    }

    [[nodiscard]] std::size_t approximate_size() const noexcept {
        return ring_.approximate_size();
    }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return QueueCapacity;
    }

    [[nodiscard]] static constexpr std::size_t max_payload_bytes() noexcept {
        return MaxPayloadBytes;
    }

private:
    SpscRing<Entry, QueueCapacity> ring_{};
};

} // namespace fgep::execution