#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/moldudp64/moldudp64_packet.hpp"

#include <cstddef>
#include <span>

namespace fgep::moldudp64 {

[[nodiscard]] ErrorCode encode_request_packet(
    std::span<std::byte> bytes,
    const RequestPacket& packet
) noexcept;

} // namespace fgep::moldudp64