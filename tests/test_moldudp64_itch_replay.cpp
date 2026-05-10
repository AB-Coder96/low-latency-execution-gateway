#include "fgep/itch/itch_wire_messages.hpp"
#include "fgep/replay/moldudp64_itch_replay.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <span>
#include <variant>

namespace {

[[nodiscard]] constexpr std::byte ub(unsigned int value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

[[nodiscard]] constexpr std::byte cb(char value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

} // namespace

int main() {
    using namespace fgep;

    {
        const std::array<std::byte, 72> packet_bytes{
            // MoldUDP64 session: "ITCH000001"
            cb('I'), cb('T'), cb('C'), cb('H'), cb('0'),
            cb('0'), cb('0'), cb('0'), cb('0'), cb('1'),

            // MoldUDP64 first sequence number = 500
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x01), ub(0xF4),

            // MoldUDP64 message count = 2
            ub(0x00), ub(0x02),

            // Message block 0 length = 12
            ub(0x00), ub(0x0C),

            // ITCH S - System Event
            cb('S'),
            ub(0x00), ub(0x00),                         // stock locate = 0
            ub(0x00), ub(0x01),                         // tracking number = 1
            ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x01),               // timestamp = 1
            cb('O'),                                    // start of messages

            // Message block 1 length = 36
            ub(0x00), ub(0x24),

            // ITCH A - Add Order, No MPID Attribution
            cb('A'),
            ub(0x00), ub(0x01),                         // stock locate = 1
            ub(0x00), ub(0x02),                         // tracking number = 2
            ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x03), ub(0xE8),               // timestamp = 1000

            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x64),     // order ref = 100

            cb('B'),                                    // buy

            ub(0x00), ub(0x00), ub(0x00), ub(0xFA),     // shares = 250

            cb('A'), cb('A'), cb('P'), cb('L'),
            cb(' '), cb(' '), cb(' '), cb(' '),         // stock = "AAPL"

            ub(0x00), ub(0x1D), ub(0x07), ub(0xA4)      // price = 1902500
        };

        const auto result = replay::decode_moldudp64_itch_packet(
            std::span<const std::byte>{packet_bytes}
        );

        assert(result.ok());

        const auto& packet = result.value;

        assert(packet.kind == replay::ReplayPacketKind::data);
        assert(packet.first_sequence_number == 500);
        assert(packet.message_count == 2);
        assert(packet.messages.size() == 2);

        assert(packet.messages[0].sequence_number == 500);

        const auto* system_event = std::get_if<itch::SystemEventMessage>(
            &packet.messages[0].message
        );

        assert(system_event != nullptr);
        assert(system_event->header.stock_locate == 0);
        assert(system_event->header.tracking_number == 1);
        assert(system_event->header.timestamp_ns == 1);
        assert(
            system_event->event_code
            == itch::SystemEventCode::start_of_messages
        );

        assert(packet.messages[1].sequence_number == 501);

        const auto* add_order = std::get_if<itch::AddOrderNoMpidMessage>(
            &packet.messages[1].message
        );

        assert(add_order != nullptr);
        assert(add_order->header.stock_locate == 1);
        assert(add_order->header.tracking_number == 2);
        assert(add_order->header.timestamp_ns == 1000);
        assert(add_order->order_reference_number == 100);
        assert(add_order->side == itch::Side::buy);
        assert(add_order->shares == 250);
        assert(wire::fixed_ascii_equals(add_order->stock, "AAPL"));
        assert(add_order->price == 1902500);
    }

    {
        const std::array<std::byte, moldudp64::length_downstream_header> bytes{
            cb('I'), cb('T'), cb('C'), cb('H'), cb('0'),
            cb('0'), cb('0'), cb('0'), cb('0'), cb('1'),

            // next expected sequence number = 777
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x03), ub(0x09),

            // heartbeat message count = 0
            ub(0x00), ub(0x00)
        };

        const auto result = replay::decode_moldudp64_itch_packet(bytes);

        assert(result.ok());
        assert(result.value.kind == replay::ReplayPacketKind::heartbeat);
        assert(result.value.first_sequence_number == 777);
        assert(result.value.message_count == 0);
        assert(result.value.messages.empty());
    }

    {
        const std::array<std::byte, 23> bytes{
            cb('I'), cb('T'), cb('C'), cb('H'), cb('0'),
            cb('0'), cb('0'), cb('0'), cb('0'), cb('1'),

            // first sequence number = 1
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x01),

            // message count = 1
            ub(0x00), ub(0x01),

            // message block length = 1
            ub(0x00), ub(0x01),

            // invalid/unknown ITCH message type
            cb('?')
        };

        const auto result = replay::decode_moldudp64_itch_packet(bytes);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    return 0;
}