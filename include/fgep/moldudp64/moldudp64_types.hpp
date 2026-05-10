#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace fgep::moldudp64 {

using Session = std::array<std::byte, 10>;
using SequenceNumber = std::uint64_t;
using MessageCount = std::uint16_t;
using MessageLength = std::uint16_t;
using RequestedMessageCount = std::uint16_t;

inline constexpr std::size_t length_session = 10;

inline constexpr std::size_t length_downstream_header = 20;
inline constexpr std::size_t offset_session = 0;
inline constexpr std::size_t offset_sequence_number = 10;
inline constexpr std::size_t offset_message_count = 18;

inline constexpr std::size_t length_request_packet = 20;
inline constexpr std::size_t offset_request_session = 0;
inline constexpr std::size_t offset_request_sequence_number = 10;
inline constexpr std::size_t offset_requested_message_count = 18;

inline constexpr MessageCount heartbeat_message_count = 0;
inline constexpr MessageCount end_of_session_message_count = 0xFFFFU;

} // namespace fgep::moldudp64