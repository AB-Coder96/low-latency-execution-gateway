#include "fgep/moldudp64/moldudp64_encode.hpp"

#include "fgep/wire/byte_io.hpp"

#include <algorithm>
#include <cstddef>

namespace fgep::moldudp64 {

namespace {

[[nodiscard]] ErrorCode write_session(
    std::span<std::byte> bytes,
    std::size_t offset,
    const Session& session
) noexcept {
    if (!wire::has_range(bytes, offset, length_session)) {
        return ErrorCode::parse_error;
    }

    std::copy_n(
        session.begin(),
        length_session,
        bytes.begin() + static_cast<std::ptrdiff_t>(offset)
    );

    return ErrorCode::ok;
}

} // namespace

ErrorCode encode_request_packet(
    std::span<std::byte> bytes,
    const RequestPacket& packet
) noexcept {
    if (bytes.size() != length_request_packet) {
        return ErrorCode::parse_error;
    }

    auto error = write_session(bytes, offset_request_session, packet.session);
    if (error != ErrorCode::ok) {
        return error;
    }

    error = wire::write_u64_be(
        bytes,
        offset_request_sequence_number,
        packet.sequence_number
    );
    if (error != ErrorCode::ok) {
        return error;
    }

    return wire::write_u16_be(
        bytes,
        offset_requested_message_count,
        packet.requested_message_count
    );
}

} // namespace fgep::moldudp64