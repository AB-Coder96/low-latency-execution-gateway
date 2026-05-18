#include "fgep/core/spsc_ring.hpp"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <thread>

namespace {

struct Payload {
    std::uint64_t sequence{};
    std::uint64_t checksum{};
};

[[nodiscard]] constexpr std::uint64_t checksum_for(
    std::uint64_t value
) noexcept {
    return value ^ 0x9e3779b97f4a7c15ULL;
}

void test_empty_full_and_wraparound() {
    fgep::SpscRing<std::uint64_t, 4U> ring{};

    assert(ring.empty());
    assert(ring.approximate_size() == 0U);
    assert(ring.capacity() == 4U);

    std::uint64_t value = 0U;
    assert(!ring.pop(value));

    assert(ring.push(1U));
    assert(ring.push(2U));
    assert(ring.push(3U));
    assert(ring.push(4U));
    assert(!ring.push(5U));
    assert(ring.approximate_size() == 4U);

    assert(ring.pop(value));
    assert(value == 1U);
    assert(ring.pop(value));
    assert(value == 2U);

    assert(ring.push(5U));
    assert(ring.push(6U));
    assert(!ring.push(7U));

    assert(ring.pop(value));
    assert(value == 3U);
    assert(ring.pop(value));
    assert(value == 4U);
    assert(ring.pop(value));
    assert(value == 5U);
    assert(ring.pop(value));
    assert(value == 6U);
    assert(!ring.pop(value));
    assert(ring.empty());
}

void test_acquire_release_stress() {
    fgep::SpscRing<Payload, 1024U> ring{};

    static_assert(alignof(decltype(ring)) == fgep::cache_line_size);
    static_assert(decltype(ring)::capacity() == 1024U);

    constexpr std::uint64_t message_count = 10'000'000ULL;

    std::atomic<bool> start{false};

    std::thread producer{[&ring, &start]() {
        while (!start.load(std::memory_order_acquire)) {
        }

        for (std::uint64_t sequence = 1U;
             sequence <= message_count;
             ++sequence) {
            const Payload payload{
                .sequence = sequence,
                .checksum = checksum_for(sequence)
            };

            while (!ring.push(payload)) {
            }
        }
    }};

    std::thread consumer{[&ring, &start]() {
        while (!start.load(std::memory_order_acquire)) {
        }

        for (std::uint64_t expected = 1U;
             expected <= message_count;
             ++expected) {
            Payload payload{};

            while (!ring.pop(payload)) {
            }

            assert(payload.sequence == expected);
            assert(payload.checksum == checksum_for(expected));
        }
    }};

    start.store(true, std::memory_order_release);

    producer.join();
    consumer.join();

    assert(ring.empty());

    std::cout << "SPSC acquire/release stress test passed\n";
    std::cout << "messages=" << message_count << '\n';
    std::cout << "capacity=" << decltype(ring)::capacity() << '\n';
}

} // namespace

int main() {
    test_empty_full_and_wraparound();
    test_acquire_release_stress();
    return 0;
}