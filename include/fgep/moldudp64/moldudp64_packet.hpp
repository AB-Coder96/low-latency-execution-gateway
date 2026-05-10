#pragma once

#include "fgep/moldudp64/moldudp64_types.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace fgep::moldudp64 {

enum class PacketKind {
    data,
    heartbeat,
    end_of_session
};

struct MessageBlock {
    SequenceNumber sequence_number{};
    std::span<const std::byte> payload{};
};

struct DownstreamPacket {
    Session session{};
    SequenceNumber first_sequence_number{};
    MessageCount message_count{};
    PacketKind kind{PacketKind::data};
    std::vector<MessageBlock> messages{};
};

struct RequestPacket {
    Session session{};
    SequenceNumber sequence_number{};
    RequestedMessageCount requested_message_count{};
};

[[nodiscard]] constexpr bool is_special_message_count(
    MessageCount message_count
) noexcept {
    return message_count == heartbeat_message_count
        || message_count == end_of_session_message_count;
}

[[nodiscard]] constexpr PacketKind packet_kind_from_message_count(
    MessageCount message_count
) noexcept {
    if (message_count == heartbeat_message_count) {
        return PacketKind::heartbeat;
    }

    if (message_count == end_of_session_message_count) {
        return PacketKind::end_of_session;
    }

    return PacketKind::data;
}

} // namespace fgep::moldudp64