#include "fgep/replay/moldudp64_itch_replay.hpp"

#include "fgep/itch/itch_decode.hpp"
#include "fgep/moldudp64/moldudp64_decode.hpp"

namespace fgep::replay {

Result<MoldUdp64ItchReplayPacket> decode_moldudp64_itch_packet(
    std::span<const std::byte> bytes
) {
    MoldUdp64ItchReplayPacket replay_packet{};

    const auto mold_result = moldudp64::decode_downstream_packet(bytes);

    if (mold_result.failed()) {
        return {mold_result.error, replay_packet};
    }

    const auto& mold_packet = mold_result.value;

    replay_packet.session = mold_packet.session;
    replay_packet.first_sequence_number = mold_packet.first_sequence_number;
    replay_packet.message_count = mold_packet.message_count;
    replay_packet.kind = to_replay_packet_kind(mold_packet.kind);

    if (mold_packet.kind != moldudp64::PacketKind::data) {
        return {ErrorCode::ok, replay_packet};
    }

    replay_packet.messages.reserve(mold_packet.messages.size());

    for (const auto& mold_message : mold_packet.messages) {
        const auto itch_result = itch::decode_message(mold_message.payload);

        if (itch_result.failed()) {
            return {itch_result.error, replay_packet};
        }

        replay_packet.messages.push_back(
            DecodedItchMessage{
                mold_message.sequence_number,
                itch_result.value
            }
        );
    }

    return {ErrorCode::ok, replay_packet};
}

} // namespace fgep::replay