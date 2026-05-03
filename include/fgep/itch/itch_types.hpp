#pragma once

#include "fgep/core/types.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cstddef>
#include <cstdint>

namespace fgep::itch {

// -----------------------------------------------------------------------------
// Nasdaq TotalView-ITCH 5.0 wire-domain types
// -----------------------------------------------------------------------------
//
// TotalView-ITCH 5.0 wire facts represented here:
//   - integer fields are big-endian binary values
//   - alpha fields are fixed-width ASCII, left-justified, right-padded spaces
//   - timestamps are 6-byte unsigned integers: nanoseconds since midnight
//   - Stock Locate is a 2-byte integer used as a fast instrument key
//   - Tracking Number is a 2-byte Nasdaq internal tracking number
//   - order reference numbers are 8-byte unsigned integers
//   - visible order prices are usually Price(4), encoded as 4-byte integers
//
// -----------------------------------------------------------------------------
// Basic ITCH scalar types
// -----------------------------------------------------------------------------

using StockLocate = std::uint16_t;
using TrackingNumber = std::uint16_t;

using TimestampNs = fgep::TimestampNs;

using OrderReferenceNumber = std::uint64_t;
using MatchNumber = std::uint64_t;

using Shares = std::uint32_t;

// Price(4): unsigned integer with 4 implied decimal places.
// Example:
//   1012500 means 101.2500
using Price4 = std::uint32_t;

// Price(8): unsigned integer with 8 implied decimal places.
// Some administrative messages use Price(8).
using Price8 = std::uint64_t;

// Fixed-width ASCII fields.
using StockSymbol = wire::FixedAscii<8>;
using Mpid = wire::FixedAscii<4>;
using ReasonCode4 = wire::FixedAscii<4>;
using IssueSubType = wire::FixedAscii<2>;

// -----------------------------------------------------------------------------
// ITCH timestamp limits
// -----------------------------------------------------------------------------

inline constexpr TimestampNs nanoseconds_per_second = 1'000'000'000ULL;
inline constexpr TimestampNs seconds_per_day = 86'400ULL;
inline constexpr TimestampNs nanoseconds_per_day =
    seconds_per_day * nanoseconds_per_second;

// ITCH timestamps are nanoseconds since midnight.
// The maximum valid timestamp inside a normal day is one nanosecond before the
// next midnight.
inline constexpr TimestampNs max_timestamp_ns_since_midnight =
    nanoseconds_per_day - 1ULL;

// -----------------------------------------------------------------------------
// ITCH header layout
// -----------------------------------------------------------------------------
// The message-specific body begins at offset 11.

inline constexpr std::size_t offset_message_type = 0;
inline constexpr std::size_t offset_stock_locate = 1;
inline constexpr std::size_t offset_tracking_number = 3;
inline constexpr std::size_t offset_timestamp = 5;
inline constexpr std::size_t offset_message_body = 11;

inline constexpr std::size_t size_message_type = 1;
inline constexpr std::size_t size_stock_locate = 2;
inline constexpr std::size_t size_tracking_number = 2;
inline constexpr std::size_t size_timestamp = 6;
inline constexpr std::size_t size_common_header = 11;

// -----------------------------------------------------------------------------
// Message lengths for first byte-to-byte implementation set
// -----------------------------------------------------------------------------
//
// These are exact payload message lengths for the ITCH messages we will decode
// and encode first.

inline constexpr std::size_t length_system_event = 12;                 // S
inline constexpr std::size_t length_add_order_no_mpid = 36;            // A
inline constexpr std::size_t length_add_order_with_mpid = 40;          // F
inline constexpr std::size_t length_order_executed = 31;               // E
inline constexpr std::size_t length_order_executed_with_price = 36;    // C
inline constexpr std::size_t length_order_cancel = 23;                 // X
inline constexpr std::size_t length_order_delete = 19;                 // D
inline constexpr std::size_t length_order_replace = 35;                // U
inline constexpr std::size_t length_trade_non_cross = 44;              // P
inline constexpr std::size_t length_cross_trade = 40;                  // Q
inline constexpr std::size_t length_broken_trade = 19;                 // B

// -----------------------------------------------------------------------------
// ITCH header struct (decoded representation of the common header fields)
// -----------------------------------------------------------------------------
//
struct Header {
    StockLocate stock_locate{};
    TrackingNumber tracking_number{};
    TimestampNs timestamp_ns{};
};

// -----------------------------------------------------------------------------
// Validation helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr bool is_valid_stock_locate(
    StockLocate stock_locate
) noexcept {
    return stock_locate > 0;
}

[[nodiscard]] constexpr bool is_valid_or_zero_stock_locate(
    StockLocate stock_locate
) noexcept {
    return stock_locate >= 0;
}

[[nodiscard]] constexpr bool is_valid_timestamp_ns(
    TimestampNs timestamp_ns
) noexcept {
    return timestamp_ns <= max_timestamp_ns_since_midnight;
}

[[nodiscard]] constexpr bool is_valid_header_for_stock_message(
    const Header& header
) noexcept {
    return is_valid_stock_locate(header.stock_locate)
        && is_valid_timestamp_ns(header.timestamp_ns);
}

[[nodiscard]] constexpr bool is_valid_header_allowing_zero_stock_locate(
    const Header& header
) noexcept {
    return is_valid_or_zero_stock_locate(header.stock_locate)
        && is_valid_timestamp_ns(header.timestamp_ns);
}

[[nodiscard]] constexpr bool is_valid_order_reference_number(
    OrderReferenceNumber order_reference_number
) noexcept {
    return order_reference_number > 0;
}

[[nodiscard]] constexpr bool is_valid_match_number(
    MatchNumber match_number
) noexcept {
    return match_number > 0;
}

[[nodiscard]] constexpr bool is_valid_shares(Shares shares) noexcept {
    return shares > 0;
}

[[nodiscard]] constexpr bool is_valid_price4(Price4 price) noexcept {
    return price > 0;
}

[[nodiscard]] constexpr bool is_valid_price8(Price8 price) noexcept {
    return price > 0;
}

} // namespace fgep::itch