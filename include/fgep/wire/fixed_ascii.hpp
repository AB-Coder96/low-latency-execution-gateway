#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/wire/byte_io.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace fgep::wire {

// -----------------------------------------------------------------------------
// Fixed-width ASCII helpers
// -----------------------------------------------------------------------------
//
// ITCH and OUCH both use fixed-width alpha fields.
//
// Protocol rule:
//   Fixed-width alpha fields are left-justified and padded on the right with
//   spaces.
//
// Examples:
//   Symbol "AAPL" in an 8-byte field:
//     {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '}
//
//   MPID "NSDQ" in a 4-byte field:
//     {'N', 'S', 'D', 'Q'}
//
// This file keeps fixed ASCII handling explicit and byte-to-byte safe.
// It does not allocate and it does not reinterpret raw bytes as structs.

// -----------------------------------------------------------------------------
// Type alias
// -----------------------------------------------------------------------------

template <std::size_t N>
using FixedAscii = std::array<char, N>;

// -----------------------------------------------------------------------------
// ASCII validation
// -----------------------------------------------------------------------------
//
// For now, this accepts printable ASCII plus spaces.
// Protocol-specific code can apply stricter rules later if needed.

[[nodiscard]] constexpr bool is_printable_ascii_or_space(char value) noexcept {
    const auto byte = static_cast<unsigned char>(value);
    return byte >= 0x20U && byte <= 0x7EU;
}

template <std::size_t N>
[[nodiscard]] constexpr bool is_valid_fixed_ascii(
    const FixedAscii<N>& value
) noexcept {
    for (const char character : value) {
        if (!is_printable_ascii_or_space(character)) {
            return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------------------
// Read fixed-width ASCII from raw bytes
// -----------------------------------------------------------------------------

template <std::size_t N>
[[nodiscard]] inline Result<FixedAscii<N>> read_fixed_ascii(
    std::span<const std::byte> bytes,
    std::size_t offset
) noexcept {
    FixedAscii<N> value{};

    if (!has_range(bytes, offset, N)) {
        return {ErrorCode::parse_error, value};
    }

    for (std::size_t index = 0; index < N; ++index) {
        value[index] = static_cast<char>(to_u8(bytes[offset + index]));
    }

    if (!is_valid_fixed_ascii(value)) {
        return {ErrorCode::parse_error, value};
    }

    return {ErrorCode::ok, value};
}

// -----------------------------------------------------------------------------
// Write fixed-width ASCII to raw bytes
// -----------------------------------------------------------------------------

template <std::size_t N>
[[nodiscard]] inline ErrorCode write_fixed_ascii(
    std::span<std::byte> bytes,
    std::size_t offset,
    const FixedAscii<N>& value
) noexcept {
    if (!has_range(bytes, offset, N)) {
        return ErrorCode::parse_error;
    }

    if (!is_valid_fixed_ascii(value)) {
        return ErrorCode::invalid_argument;
    }

    for (std::size_t index = 0; index < N; ++index) {
        bytes[offset + index] = to_byte(
            static_cast<std::uint8_t>(
                static_cast<unsigned char>(value[index])
            )
        );
    }

    return ErrorCode::ok;
}

// -----------------------------------------------------------------------------
// Build fixed-width ASCII from a string_view
// -----------------------------------------------------------------------------

template <std::size_t N>
[[nodiscard]] inline Result<FixedAscii<N>> make_fixed_ascii(
    std::string_view text
) noexcept {
    FixedAscii<N> value{};

    if (text.size() > N) {
        return {ErrorCode::invalid_argument, value};
    }

    for (std::size_t index = 0; index < N; ++index) {
        value[index] = ' ';
    }

    for (std::size_t index = 0; index < text.size(); ++index) {
        const char character = text[index];

        if (!is_printable_ascii_or_space(character)) {
            return {ErrorCode::invalid_argument, value};
        }

        value[index] = character;
    }

    return {ErrorCode::ok, value};
}

// -----------------------------------------------------------------------------
// Convert fixed-width ASCII to trimmed string_view boundaries
// -----------------------------------------------------------------------------
//
// This returns the logical length after removing right-side space padding.
// It does not allocate.

template <std::size_t N>
[[nodiscard]] constexpr std::size_t trimmed_length(
    const FixedAscii<N>& value
) noexcept {
    std::size_t length = N;

    while (length > 0 && value[length - 1] == ' ') {
        --length;
    }

    return length;
}

// -----------------------------------------------------------------------------
// Equality helper for tests and protocol checks
// -----------------------------------------------------------------------------

template <std::size_t N>
[[nodiscard]] constexpr bool fixed_ascii_equals(
    const FixedAscii<N>& value,
    std::string_view text
) noexcept {
    if (text.size() != trimmed_length(value)) {
        return false;
    }

    for (std::size_t index = 0; index < text.size(); ++index) {
        if (value[index] != text[index]) {
            return false;
        }
    }

    return true;
}

} // namespace fgep::wire