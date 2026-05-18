#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace fgep {

inline constexpr std::size_t core_stats_cache_line_size = 64U;

enum class CoreCounter : std::uint8_t {
    rx_packets,
    tx_packets,
    rx_dropped,
    tx_dropped,
    rx_nombuf,
    submit_queued,
    submit_rejected,
    ring_full,
    backend_accepted,
    backend_rejected,
    count
};

inline constexpr std::size_t core_counter_count{
    static_cast<std::size_t>(CoreCounter::count)
};

[[nodiscard]] constexpr std::size_t core_counter_index(
    CoreCounter counter
) noexcept {
    return static_cast<std::size_t>(counter);
}

struct CoreStatsSnapshot {
    std::array<std::uint64_t, core_counter_count> counters{};

    [[nodiscard]] std::uint64_t value(CoreCounter counter) const noexcept {
        return counters[core_counter_index(counter)];
    }
};

class alignas(core_stats_cache_line_size) CoreStats {
public:
    CoreStats() = default;

    CoreStats(const CoreStats&) = delete;
    CoreStats& operator=(const CoreStats&) = delete;

    CoreStats(CoreStats&&) = delete;
    CoreStats& operator=(CoreStats&&) = delete;

    void increment(
        CoreCounter counter,
        std::uint64_t amount = 1U
    ) noexcept {
        counters_[core_counter_index(counter)].fetch_add(
            amount,
            std::memory_order_relaxed
        );
    }

    [[nodiscard]] std::uint64_t load(CoreCounter counter) const noexcept {
        return counters_[core_counter_index(counter)].load(
            std::memory_order_relaxed
        );
    }

    [[nodiscard]] CoreStatsSnapshot snapshot() const noexcept {
        CoreStatsSnapshot result{};

        for (std::size_t index = 0U; index < core_counter_count; ++index) {
            result.counters[index] = counters_[index].load(
                std::memory_order_relaxed
            );
        }

        return result;
    }

    void reset() noexcept {
        for (auto& counter : counters_) {
            counter.store(0U, std::memory_order_relaxed);
        }
    }

    void on_rx_packets(std::uint64_t count) noexcept {
        increment(CoreCounter::rx_packets, count);
    }

    void on_tx_packets(std::uint64_t count) noexcept {
        increment(CoreCounter::tx_packets, count);
    }

    void on_rx_drop(std::uint64_t count = 1U) noexcept {
        increment(CoreCounter::rx_dropped, count);
    }

    void on_tx_drop(std::uint64_t count = 1U) noexcept {
        increment(CoreCounter::tx_dropped, count);
    }

    void on_rx_nombuf(std::uint64_t count = 1U) noexcept {
        increment(CoreCounter::rx_nombuf, count);
    }

    void on_submit_queued() noexcept {
        increment(CoreCounter::submit_queued);
    }

    void on_submit_rejected() noexcept {
        increment(CoreCounter::submit_rejected);
    }

    void on_ring_full() noexcept {
        increment(CoreCounter::ring_full);
    }

    void on_backend_accepted() noexcept {
        increment(CoreCounter::backend_accepted);
    }

    void on_backend_rejected() noexcept {
        increment(CoreCounter::backend_rejected);
    }

private:
    std::array<std::atomic<std::uint64_t>, core_counter_count> counters_{};
};

static_assert(alignof(CoreStats) == core_stats_cache_line_size);
static_assert(sizeof(CoreStats) % core_stats_cache_line_size == 0U);

} // namespace fgep