#include "fgep/moldudp64/moldudp64_decode.hpp"
#include "fgep/moldudp64/moldudp64_encode.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <span>

namespace {

[[nodiscard]] constexpr std::byte ub(unsigned int value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

[[nodiscard]] constexpr std::byte cb(char value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

template <std::size_t N>
[[nodiscard]] fgep::moldudp64::Session first_ten(
    const std::array<std::byte, N>& bytes
) noexcept {
    fgep::moldudp64::Session session{};

    for (std::size_t index = 0; index < session.size(); ++index) {
        session[index] = bytes[index];
    }

    return session;
}

} // namespace

int main() {
    using namespace fgep;
    using namespace fgep::moldudp64;

    {
        const std::array<std::byte, 31> bytes{
            cb('S'), cb('E'), cb('S'), cb('S'), cb('I'),
            cb('O'), cb('N'), cb('0'), cb('0'), cb('1'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x0A),
            ub(0x00), ub(0x02),
            ub(0x00), ub(0x03), cb('A'), ub(0x01), ub(0x02),
            ub(0x00), ub(0x04), cb('B'), ub(0x03), ub(0x04), ub(0x05)
        };

        const auto result = decode_downstream_packet(bytes);

        assert(result.ok());

        const auto& packet = result.value;

        assert(packet.kind == PacketKind::data);
        assert(packet.session == first_ten(bytes));
        assert(packet.first_sequence_number == 10);
        assert(packet.message_count == 2);
        assert(packet.messages.size() == 2);

        assert(packet.messages[0].sequence_number == 10);
        assert(packet.messages[0].payload.size() == 3);
        assert(packet.messages[0].payload[0] == cb('A'));
        assert(packet.messages[0].payload[1] == ub(0x01));
        assert(packet.messages[0].payload[2] == ub(0x02));

        assert(packet.messages[1].sequence_number == 11);
        assert(packet.messages[1].payload.size() == 4);
        assert(packet.messages[1].payload[0] == cb('B'));
        assert(packet.messages[1].payload[1] == ub(0x03));
        assert(packet.messages[1].payload[2] == ub(0x04));
        assert(packet.messages[1].payload[3] == ub(0x05));
    }

    {
        const std::array<std::byte, length_downstream_header> bytes{
            cb('S'), cb('E'), cb('S'), cb('S'), cb('I'),
            cb('O'), cb('N'), cb('0'), cb('0'), cb('1'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x0C),
            ub(0x00), ub(0x00)
        };

        const auto result = decode_downstream_packet(bytes);

        assert(result.ok());
        assert(result.value.kind == PacketKind::heartbeat);
        assert(result.value.first_sequence_number == 12);
        assert(result.value.messages.empty());
    }

    {
        const std::array<std::byte, length_downstream_header> bytes{
            cb('S'), cb('E'), cb('S'), cb('S'), cb('I'),
            cb('O'), cb('N'), cb('0'), cb('0'), cb('1'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x0C),
            ub(0xFF), ub(0xFF)
        };

        const auto result = decode_downstream_packet(bytes);

        assert(result.ok());
        assert(result.value.kind == PacketKind::end_of_session);
        assert(result.value.first_sequence_number == 12);
        assert(result.value.messages.empty());
    }

    {
        const std::array<std::byte, 24> bytes{
            cb('S'), cb('E'), cb('S'), cb('S'), cb('I'),
            cb('O'), cb('N'), cb('0'), cb('0'), cb('1'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x01),
            ub(0x00), ub(0x01),
            ub(0x00), ub(0x03), cb('A'), cb('B')
        };

        const auto result = decode_downstream_packet(bytes);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    {
        const std::array<std::byte, length_downstream_header + 1> bytes{
            cb('S'), cb('E'), cb('S'), cb('S'), cb('I'),
            cb('O'), cb('N'), cb('0'), cb('0'), cb('1'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x0C),
            ub(0x00), ub(0x00),
            ub(0x99)
        };

        const auto result = decode_downstream_packet(bytes);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    {
        const std::array<std::byte, length_request_packet> bytes{
            cb('S'), cb('E'), cb('S'), cb('S'), cb('I'),
            cb('O'), cb('N'), cb('0'), cb('0'), cb('1'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x2A),
            ub(0x00), ub(0x05)
        };

        const auto decoded = decode_request_packet(bytes);

        assert(decoded.ok());
        assert(decoded.value.session == first_ten(bytes));
        assert(decoded.value.sequence_number == 42);
        assert(decoded.value.requested_message_count == 5);

        std::array<std::byte, length_request_packet> encoded{};

        assert(encode_request_packet(encoded, decoded.value) == ErrorCode::ok);
        assert(encoded == bytes);
    }

    return 0;
}