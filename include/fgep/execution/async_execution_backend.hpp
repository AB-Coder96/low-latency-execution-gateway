#pragma once

#include "fgep/execution/backend_submit_queue.hpp"
#include "fgep/execution/execution_backend.hpp"
#include "fgep/telemetry/core_stats.hpp"

#include <atomic>
#include <cstddef>
#include <thread>

namespace fgep::execution {

template <
    std::size_t Capacity,
    std::size_t MaxPayloadSize = max_backend_submit_payload_size
>
class AsyncExecutionBackend final : public ExecutionBackend {
public:
    AsyncExecutionBackend(
        ExecutionBackend& backend,
        ::fgep::CoreStats* stats = nullptr,
        bool start_worker = true
    )
        : backend_{&backend},
          stats_{stats} {
        queue_.set_stats(stats_);

        if (start_worker) {
            start();
        }
    }

    ~AsyncExecutionBackend() override {
        stop();
    }

    AsyncExecutionBackend(const AsyncExecutionBackend&) = delete;
    AsyncExecutionBackend& operator=(const AsyncExecutionBackend&) = delete;

    AsyncExecutionBackend(AsyncExecutionBackend&&) = delete;
    AsyncExecutionBackend& operator=(AsyncExecutionBackend&&) = delete;

    [[nodiscard]] BackendSubmitResult submit(
        const BackendSubmitRequest& request
    ) override {
        const auto result = queue_.push(request);

        if (!result.queued()) {
            return make_backend_reject(
                request,
                reject_reason_for(result.status)
            );
        }

        return make_backend_accept(request);
    }

    void start() {
        if (worker_.joinable()) {
            return;
        }

        stop_requested_.store(false, std::memory_order_release);

        worker_ = std::thread{[this]() {
            worker_loop();
        }};
    }

    void stop() noexcept {
        stop_requested_.store(true, std::memory_order_release);

        if (worker_.joinable()) {
            worker_.join();
        }
    }

    [[nodiscard]] bool running() const noexcept {
        return running_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool poll_once() noexcept {
        typename Queue::Item item{};

        if (!queue_.pop(item)) {
            return false;
        }

        const auto result = backend_->submit(item.request());
        record_backend_result(result);

        return true;
    }

    [[nodiscard]] std::size_t drain() noexcept {
        std::size_t processed = 0U;

        while (poll_once()) {
            ++processed;
        }

        return processed;
    }

    [[nodiscard]] bool empty() const noexcept {
        return queue_.empty();
    }

    [[nodiscard]] std::size_t approximate_size() const noexcept {
        return queue_.approximate_size();
    }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return Capacity;
    }

    [[nodiscard]] static constexpr std::size_t max_payload_size() noexcept {
        return MaxPayloadSize;
    }

private:
    using Queue = BackendSubmitQueue<Capacity, MaxPayloadSize>;

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

    void record_backend_result(
        const BackendSubmitResult& result
    ) noexcept {
        if (stats_ == nullptr) {
            return;
        }

        if (result.accepted()) {
            stats_->increment(::fgep::CoreCounter::backend_accepted);
        } else {
            stats_->increment(::fgep::CoreCounter::backend_rejected);
        }
    }

    void worker_loop() noexcept {
        running_.store(true, std::memory_order_release);

        while (
            !stop_requested_.load(std::memory_order_acquire)
            || !queue_.empty()
        ) {
            if (!poll_once()) {
                std::this_thread::yield();
            }
        }

        running_.store(false, std::memory_order_release);
    }

    ExecutionBackend* backend_{nullptr};
    Queue queue_{};
    ::fgep::CoreStats* stats_{nullptr};

    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> running_{false};
    std::thread worker_{};
};

} // namespace fgep::execution