#include "fgep/itch/itch_decode.hpp"
#include "fgep/itch/itch_wire_messages.hpp"
#include "fgep/moldudp64/moldudp64_decode.hpp"
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

    // -------------------------------------------------------------------------
    // MoldUDP64 packet containing two ITCH payload messages:
    //
    //   MoldUDP64 downstream header:
    //     session = "ITCH000001"
    //     first sequence number = 500
    //     message count = 2
    //
    //   Message block 0:
    //     ITCH S - System Event
    //
    //   Message block 1:
    //     ITCH A - Add Order, No MPID Attribution
    //
    // This proves:
    //
    //   UDP packet bytes
    //   -> moldudp64::decode_downstream_packet()
    //   -> per-message payload spans
    //   -> itch::decode_message()
    // -------------------------------------------------------------------------

    const std::array<std::byte, 72> packet_bytes{
        // MoldUDP64 session: "ITCH000001"
        cb('I'), cb('T'), cb('C'), cb('H'), cb('0'),
        cb('0'), cb('0'), cb('0'), cb('0'), cb('1'),

        // MoldUDP64 first sequence number = 500
        ub(0x00), ub(0x00), ub(0x00), ub(0x00),
        ub(0x00), ub(0x00), ub(0x01), ub(0xF4),

        // MoldUDP64 message count = 2
        ub(0x00), ub(0x02),

        // ---------------------------------------------------------------------
        // MoldUDP64 message block 0 length = 12
        // ---------------------------------------------------------------------
        ub(0x00), ub(0x0C),

        // ITCH S - System Event Message
        cb('S'),
        ub(0x00), ub(0x00),                         // stock locate = 0
        ub(0x00), ub(0x01),                         // tracking number = 1
        ub(0x00), ub(0x00), ub(0x00),
        ub(0x00), ub(0x00), ub(0x01),               // timestamp = 1
        cb('O'),                                    // start of messages

        // ---------------------------------------------------------------------
        // MoldUDP64 message block 1 length = 36
        // ---------------------------------------------------------------------
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

    const auto mold_result = fgep::moldudp64::decode_downstream_packet(
        std::span<const std::byte>{packet_bytes}
    );

    assert(mold_result.ok());

    const auto& mold_packet = mold_result.value;

    assert(mold_packet.kind == fgep::moldudp64::PacketKind::data);
    assert(mold_packet.first_sequence_number == 500);
    assert(mold_packet.message_count == 2);
    assert(mold_packet.messages.size() == 2);

    assert(mold_packet.messages[0].sequence_number == 500);
    assert(mold_packet.messages[0].payload.size() == fgep::itch::length_system_event);

    assert(mold_packet.messages[1].sequence_number == 501);
    assert(
        mold_packet.messages[1].payload.size()
        == fgep::itch::length_add_order_no_mpid
    );

    // -------------------------------------------------------------------------
    // Decode first MoldUDP64 payload as ITCH.
    // -------------------------------------------------------------------------

    const auto first_itch_result = fgep::itch::decode_message(
        mold_packet.messages[0].payload
    );

    assert(first_itch_result.ok());

    const auto* system_event = std::get_if<fgep::itch::SystemEventMessage>(
        &first_itch_result.value
    );

    assert(system_event != nullptr);
    assert(system_event->header.stock_locate == 0);
    assert(system_event->header.tracking_number == 1);
    assert(system_event->header.timestamp_ns == 1);
    assert(
        system_event->event_code
        == fgep::itch::SystemEventCode::start_of_messages
    );

    // -------------------------------------------------------------------------
    // Decode second MoldUDP64 payload as ITCH.
    // -------------------------------------------------------------------------

    const auto second_itch_result = fgep::itch::decode_message(
        mold_packet.messages[1].payload
    );

    assert(second_itch_result.ok());

    const auto* add_order = std::get_if<fgep::itch::AddOrderNoMpidMessage>(
        &second_itch_result.value
    );

    assert(add_order != nullptr);
    assert(add_order->header.stock_locate == 1);
    assert(add_order->header.tracking_number == 2);
    assert(add_order->header.timestamp_ns == 1000);
    assert(add_order->order_reference_number == 100);
    assert(add_order->side == fgep::itch::Side::buy);
    assert(add_order->shares == 250);
    assert(fgep::wire::fixed_ascii_equals(add_order->stock, "AAPL"));
    assert(add_order->price == 1902500);

    return 0;
}