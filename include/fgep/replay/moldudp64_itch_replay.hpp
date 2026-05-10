#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/itch/itch_wire_messages.hpp"
#include "fgep/moldudp64/moldudp64_packet.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace fgep::replay {

enum class ReplayPacketKind {
    data,
    heartbeat,
    end_of_session
};

struct DecodedItchMessage {
    moldudp64::SequenceNumber sequence_number{};
    itch::Message message{};
};

struct MoldUdp64ItchReplayPacket {
    moldudp64::Session session{};
    moldudp64::SequenceNumber first_sequence_number{};
    moldudp64::MessageCount message_count{};
    ReplayPacketKind kind{ReplayPacketKind::data};
    std::vector<DecodedItchMessage> messages{};
};

[[nodiscard]] constexpr ReplayPacketKind to_replay_packet_kind(
    moldudp64::PacketKind kind
) noexcept {
    switch (kind) {
        case moldudp64::PacketKind::data:
            return ReplayPacketKind::data;
        case moldudp64::PacketKind::heartbeat:
            return ReplayPacketKind::heartbeat;
        case moldudp64::PacketKind::end_of_session:
            return ReplayPacketKind::end_of_session;
    }

    return ReplayPacketKind::data;
}

[[nodiscard]] Result<MoldUdp64ItchReplayPacket> decode_moldudp64_itch_packet(
    std::span<const std::byte> bytes
);

} // namespace fgep::replay