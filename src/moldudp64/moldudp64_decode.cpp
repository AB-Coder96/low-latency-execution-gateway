#include "fgep/moldudp64/moldudp64_decode.hpp"

#include "fgep/wire/byte_io.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace fgep::moldudp64 {

namespace {

[[nodiscard]] Result<Session> read_session(
    std::span<const std::byte> bytes,
    std::size_t offset
) noexcept {
    Session session{};

    if (!wire::has_range(bytes, offset, length_session)) {
        return {ErrorCode::parse_error, session};
    }

    std::copy_n(
        bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        length_session,
        session.begin()
    );

    return {ErrorCode::ok, session};
}

[[nodiscard]] bool sequence_number_would_wrap(
    SequenceNumber first_sequence_number,
    std::size_t zero_based_message_index
) noexcept {
    return first_sequence_number
        > std::numeric_limits<SequenceNumber>::max()
            - static_cast<SequenceNumber>(zero_based_message_index);
}

} // namespace

Result<DownstreamPacket> decode_downstream_packet(
    std::span<const std::byte> bytes
) {
    DownstreamPacket packet{};

    if (!wire::has_range(bytes, 0, length_downstream_header)) {
        return {ErrorCode::parse_error, packet};
    }

    const auto session = read_session(bytes, offset_session);
    if (session.failed()) {
        return {session.error, packet};
    }

    const auto sequence_number = wire::read_u64_be(
        bytes,
        offset_sequence_number
    );
    if (sequence_number.failed()) {
        return {sequence_number.error, packet};
    }

    const auto message_count = wire::read_u16_be(bytes, offset_message_count);
    if (message_count.failed()) {
        return {message_count.error, packet};
    }

    packet.session = session.value;
    packet.first_sequence_number = sequence_number.value;
    packet.message_count = message_count.value;
    packet.kind = packet_kind_from_message_count(packet.message_count);

    if (is_special_message_count(packet.message_count)) {
        if (bytes.size() != length_downstream_header) {
            return {ErrorCode::parse_error, packet};
        }

        return {ErrorCode::ok, packet};
    }

    packet.messages.reserve(static_cast<std::size_t>(packet.message_count));

    std::size_t offset = length_downstream_header;

    for (
        std::size_t index = 0;
        index < static_cast<std::size_t>(packet.message_count);
        ++index
    ) {
        if (sequence_number_would_wrap(packet.first_sequence_number, index)) {
            return {ErrorCode::parse_error, packet};
        }

        const auto message_length = wire::read_u16_be(bytes, offset);
        if (message_length.failed()) {
            return {message_length.error, packet};
        }

        offset += sizeof(MessageLength);

        if (!wire::has_range(bytes, offset, message_length.value)) {
            return {ErrorCode::parse_error, packet};
        }

        packet.messages.push_back(
            MessageBlock{
                packet.first_sequence_number
                    + static_cast<SequenceNumber>(index),
                bytes.subspan(offset, message_length.value)
            }
        );

        offset += message_length.value;
    }

    if (offset != bytes.size()) {
        return {ErrorCode::parse_error, packet};
    }

    return {ErrorCode::ok, packet};
}

Result<RequestPacket> decode_request_packet(
    std::span<const std::byte> bytes
) noexcept {
    RequestPacket packet{};

    if (bytes.size() != length_request_packet) {
        return {ErrorCode::parse_error, packet};
    }

    const auto session = read_session(bytes, offset_request_session);
    if (session.failed()) {
        return {session.error, packet};
    }

    const auto sequence_number = wire::read_u64_be(
        bytes,
        offset_request_sequence_number
    );
    if (sequence_number.failed()) {
        return {sequence_number.error, packet};
    }

    const auto requested_message_count = wire::read_u16_be(
        bytes,
        offset_requested_message_count
    );
    if (requested_message_count.failed()) {
        return {requested_message_count.error, packet};
    }

    packet.session = session.value;
    packet.sequence_number = sequence_number.value;
    packet.requested_message_count = requested_message_count.value;

    return {ErrorCode::ok, packet};
}

} // namespace fgep::moldudp64