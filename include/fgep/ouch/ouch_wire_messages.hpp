#pragma once

#include "fgep/ouch/ouch_message_type.hpp"
#include "fgep/ouch/ouch_types.hpp"

#include <cstddef>
#include <variant>
#include <vector>

namespace fgep::ouch {

enum class Side : char {
    buy = 'B',
    sell = 'S',
    sell_short = 'T',
    sell_short_exempt = 'E'
};

enum class TimeInForce : char {
    day = '0',
    ioc = '3',
    gtx = '5',
    gtt = '6',
    after_hours = 'E'
};

enum class Display : char {
    visible = 'Y',
    hidden = 'N',
    attributable = 'A'
};

enum class Capacity : char {
    agency = 'A',
    principal = 'P',
    riskless = 'R',
    other = 'O'
};

enum class IntermarketSweepEligibility : char {
    eligible = 'Y',
    not_eligible = 'N'
};

enum class CrossType : char {
    continuous_market = 'N',
    opening_cross = 'O',
    closing_cross = 'C',
    halt_or_ipo = 'H',
    supplemental = 'S',
    retail = 'R',
    extended_life = 'E',
    after_hours_close = 'A'
};

struct EnterOrderMessage {
    UserRefNum user_ref_num{};
    Side side{Side::buy};
    Quantity quantity{};
    Symbol symbol{};
    Price4 price{};
    TimeInForce time_in_force{TimeInForce::day};
    Display display{Display::visible};
    Capacity capacity{Capacity::agency};
    IntermarketSweepEligibility intermarket_sweep_eligibility{
        IntermarketSweepEligibility::not_eligible
    };
    CrossType cross_type{CrossType::continuous_market};
    ClOrdId cl_ord_id{};
    std::vector<std::byte> optional_appendage{};
};

struct ReplaceOrderMessage {
    UserRefNum orig_user_ref_num{};
    UserRefNum user_ref_num{};
    Quantity quantity{};
    Price4 price{};
    TimeInForce time_in_force{TimeInForce::day};
    Display display{Display::visible};
    IntermarketSweepEligibility intermarket_sweep_eligibility{
        IntermarketSweepEligibility::not_eligible
    };
    ClOrdId cl_ord_id{};
    std::vector<std::byte> optional_appendage{};
};

struct CancelOrderMessage {
    UserRefNum user_ref_num{};
    Quantity quantity{};
    std::vector<std::byte> optional_appendage{};
};

using Message = std::variant<
    EnterOrderMessage,
    ReplaceOrderMessage,
    CancelOrderMessage
>;

[[nodiscard]] constexpr char to_char(Side value) noexcept {
    return static_cast<char>(value);
}

[[nodiscard]] constexpr char to_char(TimeInForce value) noexcept {
    return static_cast<char>(value);
}

[[nodiscard]] constexpr char to_char(Display value) noexcept {
    return static_cast<char>(value);
}

[[nodiscard]] constexpr char to_char(Capacity value) noexcept {
    return static_cast<char>(value);
}

[[nodiscard]] constexpr char to_char(
    IntermarketSweepEligibility value
) noexcept {
    return static_cast<char>(value);
}

[[nodiscard]] constexpr char to_char(CrossType value) noexcept {
    return static_cast<char>(value);
}

[[nodiscard]] constexpr Result<Side> side_from_char(char value) noexcept {
    switch (value) {
        case 'B':
            return {ErrorCode::ok, Side::buy};
        case 'S':
            return {ErrorCode::ok, Side::sell};
        case 'T':
            return {ErrorCode::ok, Side::sell_short};
        case 'E':
            return {ErrorCode::ok, Side::sell_short_exempt};
        default:
            return {ErrorCode::parse_error, Side::buy};
    }
}

[[nodiscard]] constexpr Result<TimeInForce> time_in_force_from_char(
    char value
) noexcept {
    switch (value) {
        case '0':
            return {ErrorCode::ok, TimeInForce::day};
        case '3':
            return {ErrorCode::ok, TimeInForce::ioc};
        case '5':
            return {ErrorCode::ok, TimeInForce::gtx};
        case '6':
            return {ErrorCode::ok, TimeInForce::gtt};
        case 'E':
            return {ErrorCode::ok, TimeInForce::after_hours};
        default:
            return {ErrorCode::parse_error, TimeInForce::day};
    }
}

[[nodiscard]] constexpr Result<Display> display_from_char(char value) noexcept {
    switch (value) {
        case 'Y':
            return {ErrorCode::ok, Display::visible};
        case 'N':
            return {ErrorCode::ok, Display::hidden};
        case 'A':
            return {ErrorCode::ok, Display::attributable};
        default:
            return {ErrorCode::parse_error, Display::visible};
    }
}

[[nodiscard]] constexpr Result<Capacity> capacity_from_char(char value) noexcept {
    switch (value) {
        case 'A':
            return {ErrorCode::ok, Capacity::agency};
        case 'P':
            return {ErrorCode::ok, Capacity::principal};
        case 'R':
            return {ErrorCode::ok, Capacity::riskless};
        case 'O':
            return {ErrorCode::ok, Capacity::other};
        default:
            return {ErrorCode::parse_error, Capacity::agency};
    }
}

[[nodiscard]] constexpr Result<IntermarketSweepEligibility>
intermarket_sweep_eligibility_from_char(char value) noexcept {
    switch (value) {
        case 'Y':
            return {ErrorCode::ok, IntermarketSweepEligibility::eligible};
        case 'N':
            return {ErrorCode::ok, IntermarketSweepEligibility::not_eligible};
        default:
            return {
                ErrorCode::parse_error,
                IntermarketSweepEligibility::not_eligible
            };
    }
}

[[nodiscard]] constexpr Result<CrossType> cross_type_from_char(
    char value
) noexcept {
    switch (value) {
        case 'N':
            return {ErrorCode::ok, CrossType::continuous_market};
        case 'O':
            return {ErrorCode::ok, CrossType::opening_cross};
        case 'C':
            return {ErrorCode::ok, CrossType::closing_cross};
        case 'H':
            return {ErrorCode::ok, CrossType::halt_or_ipo};
        case 'S':
            return {ErrorCode::ok, CrossType::supplemental};
        case 'R':
            return {ErrorCode::ok, CrossType::retail};
        case 'E':
            return {ErrorCode::ok, CrossType::extended_life};
        case 'A':
            return {ErrorCode::ok, CrossType::after_hours_close};
        default:
            return {ErrorCode::parse_error, CrossType::continuous_market};
    }
}

[[nodiscard]] inline bool appendage_length_fits(
    const std::vector<std::byte>& optional_appendage
) noexcept {
    return optional_appendage.size() <= 0xFFFFU;
}

[[nodiscard]] inline bool is_valid_enter_order_message(
    const EnterOrderMessage& message
) noexcept {
    return is_valid_user_ref_num(message.user_ref_num)
        && is_valid_enter_or_replace_quantity(message.quantity)
        && is_valid_price4(message.price)
        && wire::is_valid_fixed_ascii(message.symbol)
        && wire::is_valid_fixed_ascii(message.cl_ord_id)
        && appendage_length_fits(message.optional_appendage);
}

[[nodiscard]] inline bool is_valid_replace_order_message(
    const ReplaceOrderMessage& message
) noexcept {
    return is_valid_user_ref_num(message.orig_user_ref_num)
        && is_valid_user_ref_num(message.user_ref_num)
        && message.orig_user_ref_num != message.user_ref_num
        && is_valid_enter_or_replace_quantity(message.quantity)
        && is_valid_price4(message.price)
        && wire::is_valid_fixed_ascii(message.cl_ord_id)
        && appendage_length_fits(message.optional_appendage);
}

[[nodiscard]] inline bool is_valid_cancel_order_message(
    const CancelOrderMessage& message
) noexcept {
    return is_valid_user_ref_num(message.user_ref_num)
        && is_valid_cancel_quantity(message.quantity)
        && appendage_length_fits(message.optional_appendage);
}

[[nodiscard]] inline std::size_t encoded_length(
    const EnterOrderMessage& message
) noexcept {
    return length_enter_order_base + message.optional_appendage.size();
}

[[nodiscard]] inline std::size_t encoded_length(
    const ReplaceOrderMessage& message
) noexcept {
    return length_replace_order_base + message.optional_appendage.size();
}

[[nodiscard]] inline std::size_t encoded_length(
    const CancelOrderMessage& message
) noexcept {
    return length_cancel_order_base + message.optional_appendage.size();
}

[[nodiscard]] inline std::size_t encoded_length(const Message& message) noexcept {
    return std::visit(
        [](const auto& concrete_message) noexcept -> std::size_t {
            return encoded_length(concrete_message);
        },
        message
    );
}

} // namespace fgep::ouch