#pragma once

#include "fgep/core/errors.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace fgep::wire {

// -----------------------------------------------------------------------------
// Byte-level big-endian I/O helpers
// -----------------------------------------------------------------------------
//
// This file is the shared foundation for byte-to-byte ITCH and OUCH parsing.
// -----------------------------------------------------------------------------
// Range checking
// -----------------------------------------------------------------------------

[[nodiscard]] inline bool has_range(
    std::span<const std::byte> bytes,
    std::size_t offset,
    std::size_t length
) noexcept {
    return offset <= bytes.size() && length <= bytes.size() - offset;
}

[[nodiscard]] inline bool has_range(
    std::span<std::byte> bytes,
    std::size_t offset,
    std::size_t length
) noexcept {
    return offset <= bytes.size() && length <= bytes.size() - offset;
}

// -----------------------------------------------------------------------------
// Byte conversion helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr std::uint8_t to_u8(std::byte byte) noexcept {
    return static_cast<std::uint8_t>(byte);
}

[[nodiscard]] constexpr std::byte to_byte(std::uint8_t value) noexcept {
    return static_cast<std::byte>(value);
}

// -----------------------------------------------------------------------------
// Big-endian readers
// -----------------------------------------------------------------------------

[[nodiscard]] inline Result<std::uint8_t> read_u8(
    std::span<const std::byte> bytes,
    std::size_t offset
) noexcept {
    if (!has_range(bytes, offset, 1)) {
        return {ErrorCode::parse_error, 0};
    }

    return {ErrorCode::ok, to_u8(bytes[offset])};
}

[[nodiscard]] inline Result<std::uint16_t> read_u16_be(
    std::span<const std::byte> bytes,
    std::size_t offset
) noexcept {
    if (!has_range(bytes, offset, 2)) {
        return {ErrorCode::parse_error, 0};
    }

    const auto b0 = static_cast<std::uint16_t>(to_u8(bytes[offset]));
    const auto b1 = static_cast<std::uint16_t>(to_u8(bytes[offset + 1]));

    return {
        ErrorCode::ok,
        static_cast<std::uint16_t>((b0 << 8U) | b1)
    };
}

[[nodiscard]] inline Result<std::uint32_t> read_u32_be(
    std::span<const std::byte> bytes,
    std::size_t offset
) noexcept {
    if (!has_range(bytes, offset, 4)) {
        return {ErrorCode::parse_error, 0};
    }

    const auto b0 = static_cast<std::uint32_t>(to_u8(bytes[offset]));
    const auto b1 = static_cast<std::uint32_t>(to_u8(bytes[offset + 1]));
    const auto b2 = static_cast<std::uint32_t>(to_u8(bytes[offset + 2]));
    const auto b3 = static_cast<std::uint32_t>(to_u8(bytes[offset + 3]));

    return {
        ErrorCode::ok,
        (b0 << 24U) | (b1 << 16U) | (b2 << 8U) | b3
    };
}

// ITCH timestamps are 6-byte unsigned integers.
// Some protocol fields use 6-byte numeric values, so this returns uint64_t.
[[nodiscard]] inline Result<std::uint64_t> read_u48_be(
    std::span<const std::byte> bytes,
    std::size_t offset
) noexcept {
    if (!has_range(bytes, offset, 6)) {
        return {ErrorCode::parse_error, 0};
    }

    const auto b0 = static_cast<std::uint64_t>(to_u8(bytes[offset]));
    const auto b1 = static_cast<std::uint64_t>(to_u8(bytes[offset + 1]));
    const auto b2 = static_cast<std::uint64_t>(to_u8(bytes[offset + 2]));
    const auto b3 = static_cast<std::uint64_t>(to_u8(bytes[offset + 3]));
    const auto b4 = static_cast<std::uint64_t>(to_u8(bytes[offset + 4]));
    const auto b5 = static_cast<std::uint64_t>(to_u8(bytes[offset + 5]));

    return {
        ErrorCode::ok,
        (b0 << 40U)
            | (b1 << 32U)
            | (b2 << 24U)
            | (b3 << 16U)
            | (b4 << 8U)
            | b5
    };
}

[[nodiscard]] inline Result<std::uint64_t> read_u64_be(
    std::span<const std::byte> bytes,
    std::size_t offset
) noexcept {
    if (!has_range(bytes, offset, 8)) {
        return {ErrorCode::parse_error, 0};
    }

    const auto b0 = static_cast<std::uint64_t>(to_u8(bytes[offset]));
    const auto b1 = static_cast<std::uint64_t>(to_u8(bytes[offset + 1]));
    const auto b2 = static_cast<std::uint64_t>(to_u8(bytes[offset + 2]));
    const auto b3 = static_cast<std::uint64_t>(to_u8(bytes[offset + 3]));
    const auto b4 = static_cast<std::uint64_t>(to_u8(bytes[offset + 4]));
    const auto b5 = static_cast<std::uint64_t>(to_u8(bytes[offset + 5]));
    const auto b6 = static_cast<std::uint64_t>(to_u8(bytes[offset + 6]));
    const auto b7 = static_cast<std::uint64_t>(to_u8(bytes[offset + 7]));

    return {
        ErrorCode::ok,
        (b0 << 56U)
            | (b1 << 48U)
            | (b2 << 40U)
            | (b3 << 32U)
            | (b4 << 24U)
            | (b5 << 16U)
            | (b6 << 8U)
            | b7
    };
}

// -----------------------------------------------------------------------------
// Big-endian writers
// -----------------------------------------------------------------------------

[[nodiscard]] inline ErrorCode write_u8(
    std::span<std::byte> bytes,
    std::size_t offset,
    std::uint8_t value
) noexcept {
    if (!has_range(bytes, offset, 1)) {
        return ErrorCode::parse_error;
    }

    bytes[offset] = to_byte(value);
    return ErrorCode::ok;
}

[[nodiscard]] inline ErrorCode write_u16_be(
    std::span<std::byte> bytes,
    std::size_t offset,
    std::uint16_t value
) noexcept {
    if (!has_range(bytes, offset, 2)) {
        return ErrorCode::parse_error;
    }

    bytes[offset] = to_byte(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    bytes[offset + 1] = to_byte(static_cast<std::uint8_t>(value & 0xFFU));

    return ErrorCode::ok;
}

[[nodiscard]] inline ErrorCode write_u32_be(
    std::span<std::byte> bytes,
    std::size_t offset,
    std::uint32_t value
) noexcept {
    if (!has_range(bytes, offset, 4)) {
        return ErrorCode::parse_error;
    }

    bytes[offset] = to_byte(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
    bytes[offset + 1] = to_byte(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    bytes[offset + 2] = to_byte(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    bytes[offset + 3] = to_byte(static_cast<std::uint8_t>(value & 0xFFU));

    return ErrorCode::ok;
}

[[nodiscard]] inline ErrorCode write_u48_be(
    std::span<std::byte> bytes,
    std::size_t offset,
    std::uint64_t value
) noexcept {
    if (!has_range(bytes, offset, 6)) {
        return ErrorCode::parse_error;
    }

    if (value > 0x0000FFFFFFFFFFFFULL) {
        return ErrorCode::invalid_argument;
    }

    bytes[offset] = to_byte(static_cast<std::uint8_t>((value >> 40U) & 0xFFU));
    bytes[offset + 1] = to_byte(static_cast<std::uint8_t>((value >> 32U) & 0xFFU));
    bytes[offset + 2] = to_byte(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
    bytes[offset + 3] = to_byte(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    bytes[offset + 4] = to_byte(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    bytes[offset + 5] = to_byte(static_cast<std::uint8_t>(value & 0xFFU));

    return ErrorCode::ok;
}

[[nodiscard]] inline ErrorCode write_u64_be(
    std::span<std::byte> bytes,
    std::size_t offset,
    std::uint64_t value
) noexcept {
    if (!has_range(bytes, offset, 8)) {
        return ErrorCode::parse_error;
    }

    bytes[offset] = to_byte(static_cast<std::uint8_t>((value >> 56U) & 0xFFU));
    bytes[offset + 1] = to_byte(static_cast<std::uint8_t>((value >> 48U) & 0xFFU));
    bytes[offset + 2] = to_byte(static_cast<std::uint8_t>((value >> 40U) & 0xFFU));
    bytes[offset + 3] = to_byte(static_cast<std::uint8_t>((value >> 32U) & 0xFFU));
    bytes[offset + 4] = to_byte(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
    bytes[offset + 5] = to_byte(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    bytes[offset + 6] = to_byte(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    bytes[offset + 7] = to_byte(static_cast<std::uint8_t>(value & 0xFFU));

    return ErrorCode::ok;
}

} // namespace fgep::wire