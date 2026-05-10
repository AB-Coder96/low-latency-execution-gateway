#pragma once

#include "fgep/wire/fixed_ascii.hpp"

#include <cstddef>
#include <cstdint>

namespace fgep::ouch {

using UserRefNum = std::uint32_t;
using Quantity = std::uint32_t;
using Price4 = std::uint64_t;
using AppendageLength = std::uint16_t;

using Symbol = wire::FixedAscii<8>;
using ClOrdId = wire::FixedAscii<14>;

inline constexpr Price4 max_supported_price = 1'999'999'900ULL;
inline constexpr Price4 cross_market_price = 2'147'483'647ULL;

inline constexpr std::size_t length_enter_order_base = 47;
inline constexpr std::size_t enter_order_offset_type = 0;
inline constexpr std::size_t enter_order_offset_user_ref_num = 1;
inline constexpr std::size_t enter_order_offset_side = 5;
inline constexpr std::size_t enter_order_offset_quantity = 6;
inline constexpr std::size_t enter_order_offset_symbol = 10;
inline constexpr std::size_t enter_order_offset_price = 18;
inline constexpr std::size_t enter_order_offset_time_in_force = 26;
inline constexpr std::size_t enter_order_offset_display = 27;
inline constexpr std::size_t enter_order_offset_capacity = 28;
inline constexpr std::size_t enter_order_offset_intermarket_sweep_eligibility = 29;
inline constexpr std::size_t enter_order_offset_cross_type = 30;
inline constexpr std::size_t enter_order_offset_cl_ord_id = 31;
inline constexpr std::size_t enter_order_offset_appendage_length = 45;
inline constexpr std::size_t enter_order_offset_optional_appendage = 47;

inline constexpr std::size_t length_replace_order_base = 40;
inline constexpr std::size_t replace_order_offset_type = 0;
inline constexpr std::size_t replace_order_offset_orig_user_ref_num = 1;
inline constexpr std::size_t replace_order_offset_user_ref_num = 5;
inline constexpr std::size_t replace_order_offset_quantity = 9;
inline constexpr std::size_t replace_order_offset_price = 13;
inline constexpr std::size_t replace_order_offset_time_in_force = 21;
inline constexpr std::size_t replace_order_offset_display = 22;
inline constexpr std::size_t replace_order_offset_intermarket_sweep_eligibility = 23;
inline constexpr std::size_t replace_order_offset_cl_ord_id = 24;
inline constexpr std::size_t replace_order_offset_appendage_length = 38;
inline constexpr std::size_t replace_order_offset_optional_appendage = 40;

inline constexpr std::size_t length_cancel_order_base = 11;
inline constexpr std::size_t cancel_order_offset_type = 0;
inline constexpr std::size_t cancel_order_offset_user_ref_num = 1;
inline constexpr std::size_t cancel_order_offset_quantity = 5;
inline constexpr std::size_t cancel_order_offset_appendage_length = 9;
inline constexpr std::size_t cancel_order_offset_optional_appendage = 11;

[[nodiscard]] constexpr bool is_valid_user_ref_num(UserRefNum value) noexcept {
    return value > 0;
}

[[nodiscard]] constexpr bool is_valid_enter_or_replace_quantity(
    Quantity value
) noexcept {
    return value > 0 && value < 1'000'000U;
}

[[nodiscard]] constexpr bool is_valid_cancel_quantity(Quantity value) noexcept {
    return value < 1'000'000U;
}

[[nodiscard]] constexpr bool is_valid_price4(Price4 value) noexcept {
    return value > 0;
}

} // namespace fgep::ouch