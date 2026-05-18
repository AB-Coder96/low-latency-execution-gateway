#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace fgep {

inline constexpr std::size_t cache_line_size = 64;

template <typename T, std::size_t Capacity>
class alignas(cache_line_size) SpscRing {
    static_assert(Capacity >= 2, "SpscRing capacity must be at least 2");
    static_assert(
        (Capacity & (Capacity - 1)) == 0,
        "SpscRing capacity must be a power of two"
    );
    static_assert(
        std::is_copy_assignable_v<T>,
        "SpscRing requires copy-assignable elements"
    );

public:
    SpscRing() = default;

    SpscRing(const SpscRing&) = delete;
    SpscRing& operator=(const SpscRing&) = delete;

    SpscRing(SpscRing&&) = delete;
    SpscRing& operator=(SpscRing&&) = delete;

    [[nodiscard]] bool push(const T& item) noexcept {
        const auto head = producer_.head.load(std::memory_order_relaxed);
        const auto next = head + 1U;

        if (next - producer_.cached_tail > Capacity) {
            producer_.cached_tail =
                consumer_.tail.load(std::memory_order_acquire);

            if (next - producer_.cached_tail > Capacity) {
                return false;
            }
        }

        buffer_[head & mask] = item;

        producer_.head.store(next, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool pop(T& out) noexcept {
        const auto tail = consumer_.tail.load(std::memory_order_relaxed);

        if (tail == consumer_.cached_head) {
            consumer_.cached_head =
                producer_.head.load(std::memory_order_acquire);

            if (tail == consumer_.cached_head) {
                return false;
            }
        }

        out = buffer_[tail & mask];

        consumer_.tail.store(tail + 1U, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool empty() const noexcept {
        return approximate_size() == 0U;
    }

    [[nodiscard]] std::size_t approximate_size() const noexcept {
        const auto tail = consumer_.tail.load(std::memory_order_acquire);
        const auto head = producer_.head.load(std::memory_order_acquire);
        return head - tail;
    }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return Capacity;
    }

private:
    static constexpr std::size_t mask = Capacity - 1U;

    struct alignas(cache_line_size) ProducerState {
        std::atomic<std::size_t> head{0U};
        std::size_t cached_tail{0U};
    };

    struct alignas(cache_line_size) ConsumerState {
        std::atomic<std::size_t> tail{0U};
        std::size_t cached_head{0U};
    };

    ProducerState producer_{};
    ConsumerState consumer_{};

    alignas(cache_line_size) std::array<T, Capacity> buffer_{};
};

static_assert(alignof(SpscRing<std::uint64_t, 1024U>) == cache_line_size);

} // namespace fgep