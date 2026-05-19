#include "fgep/execution/backend_submit_queue.hpp"
#include "fgep/execution/execution_backend.hpp"
#include "fgep/telemetry/core_stats.hpp"

#include <array>
#include <atomic>
#include <charconv>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <thread>

namespace {

using Queue = fgep::execution::BackendSubmitQueue<4096U, 256U>;

volatile std::sig_atomic_t stop_signal_requested = 0;

void handle_signal(int) {
    stop_signal_requested = 1;
}

[[nodiscard]] bool stop_requested() noexcept {
    return stop_signal_requested != 0;
}

struct Config {
    std::uint64_t operations{0U};
    std::size_t payload_size{64U};
};

[[nodiscard]] bool parse_u64(
    std::string_view text,
    std::uint64_t& value
) noexcept {
    const auto* first = text.data();
    const auto* last = text.data() + text.size();

    const auto result = std::from_chars(first, last, value);

    return result.ec == std::errc{} && result.ptr == last;
}

[[nodiscard]] bool parse_size(
    std::string_view text,
    std::size_t& value
) noexcept {
    std::uint64_t parsed{};

    if (!parse_u64(text, parsed)) {
        return false;
    }

    value = static_cast<std::size_t>(parsed);
    return static_cast<std::uint64_t>(value) == parsed;
}

void print_usage(const char* program) {
    std::cerr
        << "usage: " << program << " [--operations N] [--payload-size N]\n"
        << "\n"
        << "If --operations is omitted or 0, the benchmark runs until SIGTERM/SIGINT.\n"
        << "That is the intended mode for perf stat attach measurement.\n";
}

[[nodiscard]] bool parse_args(
    int argc,
    char** argv,
    Config& config
) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view arg{argv[index]};

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return false;
        }

        if (arg == "--operations") {
            if (index + 1 >= argc) {
                return false;
            }

            ++index;

            if (!parse_u64(argv[index], config.operations)) {
                return false;
            }

            continue;
        }

        if (arg == "--payload-size") {
            if (index + 1 >= argc) {
                return false;
            }

            ++index;

            if (!parse_size(argv[index], config.payload_size)) {
                return false;
            }

            continue;
        }

        return false;
    }

    return config.payload_size > 0U
        && config.payload_size <= Queue::max_payload_size();
}

void fill_payload(
    std::array<std::byte, Queue::max_payload_size()>& payload,
    std::size_t payload_size
) noexcept {
    for (std::size_t index = 0U; index < payload_size; ++index) {
        payload[index] = static_cast<std::byte>(index & 0xffU);
    }
}

[[nodiscard]] fgep::execution::BackendSubmitRequest make_request(
    fgep::ouch::UserRefNum user_ref_num,
    std::span<const std::byte> payload
) noexcept {
    return fgep::execution::BackendSubmitRequest{
        .user_ref_num = user_ref_num,
        .kind = fgep::execution::BackendOrderKind::enter,
        .payload = payload
    };
}

} // namespace

int main(int argc, char** argv) {
    Config config{};

    if (!parse_args(argc, argv, config)) {
        print_usage(argv[0]);
        return 2;
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    Queue queue{};
    fgep::CoreStats stats{};

    queue.set_stats(&stats);

    std::array<std::byte, Queue::max_payload_size()> payload{};
    fill_payload(payload, config.payload_size);

    const std::span<const std::byte> payload_view{
        payload.data(),
        config.payload_size
    };

    std::atomic<bool> start{false};
    std::atomic<bool> producer_done{false};
    std::atomic<std::uint64_t> produced{0U};
    std::atomic<std::uint64_t> consumed{0U};
    std::atomic<std::uint64_t> checksum{0U};

    std::thread producer{[&]() {
        while (!start.load(std::memory_order_acquire)) {
        }

        std::uint64_t sequence = 1U;

        while (!stop_requested()) {
            if (config.operations != 0U && sequence > config.operations) {
                break;
            }

            const auto request = make_request(
                static_cast<fgep::ouch::UserRefNum>(sequence),
                payload_view
            );

            while (!stop_requested()) {
                const auto result = queue.push(request);

                if (result.queued()) {
                    produced.fetch_add(1U, std::memory_order_relaxed);
                    ++sequence;
                    break;
                }
            }
        }

        producer_done.store(true, std::memory_order_release);
    }};

    std::thread consumer{[&]() {
        while (!start.load(std::memory_order_acquire)) {
        }

        Queue::Item item{};
        std::uint64_t local_consumed = 0U;
        std::uint64_t local_checksum = 0U;

        while (
            !producer_done.load(std::memory_order_acquire)
            || !queue.empty()
        ) {
            if (!queue.pop(item)) {
                continue;
            }

            const auto request = item.request();

            local_checksum ^= static_cast<std::uint64_t>(
                request.user_ref_num
            );
            local_checksum ^= static_cast<std::uint64_t>(
                request.payload.size()
            );

            if (!request.payload.empty()) {
                local_checksum ^= static_cast<std::uint64_t>(
                    request.payload.front()
                );
                local_checksum ^= static_cast<std::uint64_t>(
                    request.payload.back()
                );
            }

            ++local_consumed;
        }

        consumed.store(local_consumed, std::memory_order_release);
        checksum.store(local_checksum, std::memory_order_release);
    }};

    start.store(true, std::memory_order_release);

    producer.join();
    consumer.join();

    const auto snapshot = stats.snapshot();

    std::cout << "spsc_queue_perf_complete\n";
    std::cout << "payload_size=" << config.payload_size << '\n';
    std::cout << "produced=" << produced.load(std::memory_order_relaxed) << '\n';
    std::cout << "consumed=" << consumed.load(std::memory_order_relaxed) << '\n';
    std::cout << "checksum=" << checksum.load(std::memory_order_relaxed) << '\n';
    std::cout << "submit_queued="
              << snapshot.value(fgep::CoreCounter::submit_queued) << '\n';
    std::cout << "submit_rejected="
              << snapshot.value(fgep::CoreCounter::submit_rejected) << '\n';
    std::cout << "ring_full="
              << snapshot.value(fgep::CoreCounter::ring_full) << '\n';

    return produced.load(std::memory_order_relaxed)
            == consumed.load(std::memory_order_relaxed)
        ? 0
        : 1;
}