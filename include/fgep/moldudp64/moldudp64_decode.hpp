#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/moldudp64/moldudp64_packet.hpp"

#include <cstddef>
#include <span>

namespace fgep::moldudp64 {

[[nodiscard]] Result<DownstreamPacket> decode_downstream_packet(
    std::span<const std::byte> bytes
);

[[nodiscard]] Result<RequestPacket> decode_request_packet(
    std::span<const std::byte> bytes
) noexcept;

} // namespace fgep::moldudp64