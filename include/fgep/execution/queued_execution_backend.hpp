#pragma once

#include "fgep/execution/backend_submit_queue.hpp"
#include "fgep/execution/execution_backend.hpp"

#include <cstddef>

namespace fgep::execution {

template <
    std::size_t QueueCapacity,
    std::size_t MaxPayloadBytes = default_backend_submit_payload_bytes
>
class QueuedExecutionBackend final : public ExecutionBackend {
public:
    using Queue = BackendSubmitQueue<QueueCapacity, MaxPayloadBytes>;
    using Entry = typename Queue::Entry;

    QueuedExecutionBackend() = default;

    QueuedExecutionBackend(const QueuedExecutionBackend&) = delete;
    QueuedExecutionBackend& operator=(const QueuedExecutionBackend&) = delete;

    QueuedExecutionBackend(QueuedExecutionBackend&&) = delete;
    QueuedExecutionBackend& operator=(QueuedExecutionBackend&&) = delete;

    [[nodiscard]] BackendSubmitResult submit(
        const BackendSubmitRequest& request
    ) override {
        return queue_.try_push(request);
    }

    [[nodiscard]] bool try_pop(Entry& out) noexcept {
        return queue_.try_pop(out);
    }

    [[nodiscard]] bool empty() const noexcept {
        return queue_.empty();
    }

    [[nodiscard]] std::size_t approximate_size() const noexcept {
        return queue_.approximate_size();
    }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return Queue::capacity();
    }

    [[nodiscard]] static constexpr std::size_t max_payload_bytes() noexcept {
        return Queue::max_payload_bytes();
    }

    [[nodiscard]] bool drain_one_to(ExecutionBackend& backend) {
        Entry entry{};

        if (!queue_.try_pop(entry)) {
            return false;
        }

        const auto request = entry.as_request();
        (void)backend.submit(request);
        return true;
    }

    [[nodiscard]] std::size_t drain_to(
        ExecutionBackend& backend,
        std::size_t max_submissions
    ) {
        std::size_t drained = 0U;

        while (drained < max_submissions && drain_one_to(backend)) {
            ++drained;
        }

        return drained;
    }

private:
    Queue queue_{};
};

} // namespace fgep::execution