#include "fgep/telemetry/core_stats.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <thread>

namespace {

void test_alignment() {
    static_assert(
        alignof(fgep::CoreStats)
        == fgep::core_stats_cache_line_size
    );

    static_assert(
        sizeof(fgep::CoreStats)
            % fgep::core_stats_cache_line_size
        == 0U
    );

    std::array<fgep::CoreStats, 2U> per_core_stats{};

    const auto first = reinterpret_cast<std::uintptr_t>(
        &per_core_stats[0U]
    );
    const auto second = reinterpret_cast<std::uintptr_t>(
        &per_core_stats[1U]
    );

    assert(
        first % fgep::core_stats_cache_line_size == 0U
    );
    assert(
        second % fgep::core_stats_cache_line_size == 0U
    );
}

void test_named_counters() {
    fgep::CoreStats stats{};

    stats.on_rx_packets(10U);
    stats.on_tx_packets(7U);
    stats.on_rx_drop();
    stats.on_tx_drop(2U);
    stats.on_rx_nombuf(3U);
    stats.on_submit_queued();
    stats.on_submit_rejected();
    stats.on_ring_full();
    stats.on_backend_accepted();
    stats.on_backend_rejected();

    assert(
        stats.load(fgep::CoreCounter::rx_packets) == 10U
    );
    assert(
        stats.load(fgep::CoreCounter::tx_packets) == 7U
    );
    assert(
        stats.load(fgep::CoreCounter::rx_dropped) == 1U
    );
    assert(
        stats.load(fgep::CoreCounter::tx_dropped) == 2U
    );
    assert(
        stats.load(fgep::CoreCounter::rx_nombuf) == 3U
    );
    assert(
        stats.load(fgep::CoreCounter::submit_queued) == 1U
    );
    assert(
        stats.load(fgep::CoreCounter::submit_rejected) == 1U
    );
    assert(
        stats.load(fgep::CoreCounter::ring_full) == 1U
    );
    assert(
        stats.load(fgep::CoreCounter::backend_accepted) == 1U
    );
    assert(
        stats.load(fgep::CoreCounter::backend_rejected) == 1U
    );
}

void test_snapshot_and_reset() {
    fgep::CoreStats stats{};

    stats.on_rx_packets(4U);
    stats.on_tx_packets(5U);
    stats.on_ring_full();

    const auto snapshot = stats.snapshot();

    assert(
        snapshot.value(fgep::CoreCounter::rx_packets) == 4U
    );
    assert(
        snapshot.value(fgep::CoreCounter::tx_packets) == 5U
    );
    assert(
        snapshot.value(fgep::CoreCounter::ring_full) == 1U
    );

    stats.reset();

    for (std::size_t index = 0U;
         index < fgep::core_counter_count;
         ++index) {
        assert(stats.snapshot().counters[index] == 0U);
    }
}

void test_relaxed_cross_thread_scrape() {
    fgep::CoreStats stats{};

    constexpr std::uint64_t iterations = 1'000'000ULL;

    std::thread writer{[&stats]() {
        for (std::uint64_t index = 0U; index < iterations; ++index) {
            stats.on_submit_queued();
        }
    }};

    std::thread scraper{[&stats]() {
        while (
            stats.load(fgep::CoreCounter::submit_queued)
            < iterations
        ) {
            static_cast<void>(stats.snapshot());
        }
    }};

    writer.join();
    scraper.join();

    assert(
        stats.load(fgep::CoreCounter::submit_queued)
        == iterations
    );
}

} // namespace

int main() {
    test_alignment();
    test_named_counters();
    test_snapshot_and_reset();
    test_relaxed_cross_thread_scrape();

    std::cout << "Core stats test passed\n";
    return 0;
}