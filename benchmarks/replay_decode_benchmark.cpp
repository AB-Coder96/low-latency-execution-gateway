#include "hotpath_benchmark.hpp"

#include "fgep/moldudp64/moldudp64_decode.hpp"
#include "fgep/replay/moldudp64_itch_replay.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] constexpr std::byte byte(unsigned int value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

void append_char(std::vector<std::byte>& bytes, char value) {
    bytes.push_back(
        static_cast<std::byte>(static_cast<unsigned char>(value))
    );
}

void append_u16_be(std::vector<std::byte>& bytes, std::uint16_t value) {
    bytes.push_back(byte((value >> 8U) & 0xFFU));
    bytes.push_back(byte(value & 0xFFU));
}

void append_u32_be(std::vector<std::byte>& bytes, std::uint32_t value) {
    bytes.push_back(byte((value >> 24U) & 0xFFU));
    bytes.push_back(byte((value >> 16U) & 0xFFU));
    bytes.push_back(byte((value >> 8U) & 0xFFU));
    bytes.push_back(byte(value & 0xFFU));
}

void append_u48_be(std::vector<std::byte>& bytes, std::uint64_t value) {
    bytes.push_back(byte((value >> 40U) & 0xFFU));
    bytes.push_back(byte((value >> 32U) & 0xFFU));
    bytes.push_back(byte((value >> 24U) & 0xFFU));
    bytes.push_back(byte((value >> 16U) & 0xFFU));
    bytes.push_back(byte((value >> 8U) & 0xFFU));
    bytes.push_back(byte(value & 0xFFU));
}

void append_u64_be(std::vector<std::byte>& bytes, std::uint64_t value) {
    bytes.push_back(byte((value >> 56U) & 0xFFU));
    bytes.push_back(byte((value >> 48U) & 0xFFU));
    bytes.push_back(byte((value >> 40U) & 0xFFU));
    bytes.push_back(byte((value >> 32U) & 0xFFU));
    bytes.push_back(byte((value >> 24U) & 0xFFU));
    bytes.push_back(byte((value >> 16U) & 0xFFU));
    bytes.push_back(byte((value >> 8U) & 0xFFU));
    bytes.push_back(byte(value & 0xFFU));
}

void append_fixed_ascii(
    std::vector<std::byte>& bytes,
    std::string_view text,
    std::size_t width
) {
    for (std::size_t index = 0; index < width; ++index) {
        append_char(bytes, index < text.size() ? text[index] : ' ');
    }
}

[[nodiscard]] std::vector<std::byte> add_order_message() {
    std::vector<std::byte> bytes{};

    append_char(bytes, 'A');
    append_u16_be(bytes, 1);
    append_u16_be(bytes, 1);
    append_u48_be(bytes, 1'000);

    append_u64_be(bytes, 1);
    append_char(bytes, 'B');
    append_u32_be(bytes, 100);
    append_fixed_ascii(bytes, "AAPL", 8);
    append_u32_be(bytes, 1'902'500);

    return bytes;
}

[[nodiscard]] std::vector<std::byte> moldudp64_packet() {
    const auto message = add_order_message();

    std::vector<std::byte> packet{};

    append_fixed_ascii(packet, "SESSION1", 10);
    append_u64_be(packet, 1);
    append_u16_be(packet, 1);

    append_u16_be(packet, static_cast<std::uint16_t>(message.size()));
    packet.insert(packet.end(), message.begin(), message.end());

    return packet;
}

} // namespace

int main() {
    constexpr std::uint64_t iterations = 2'000'000U;

    const auto packet = moldudp64_packet();

    const auto result = fgep::benchmarks::run_hot_loop(
        "hotpath_replay_decode_moldudp64_itch",
        iterations,
        [&](std::uint64_t) {
            const auto decoded = fgep::replay::decode_moldudp64_itch_packet(
                packet
            );

            fgep::benchmarks::do_not_optimize(decoded);
            fgep::benchmarks::clobber_memory();
        }
    );

    fgep::benchmarks::print_result(result);
    return 0;
}