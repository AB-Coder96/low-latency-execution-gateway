#pragma once

#include "fgep/core/time.hpp"
#include "fgep/core/types.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <variant>

namespace fgep {

// -----------------------------------------------------------------------------
// Nasdaq OUCH 5.0 aligned order-entry event model
// -----------------------------------------------------------------------------
//
// This file defines the internal order-entry vocabulary used by the execution
// simulator and risk supervisor.
//
// It is aligned with Nasdaq OUCH 5.0, but it is not yet a byte-for-byte OUCH
// wire encoder/decoder. The goal at this stage is to model the lifecycle and
// fields cleanly so a future OUCH parser/encoder can map to these structs.
//
// OUCH purpose:
//   OUCH lets participants enter, replace, cancel, and modify their own orders,
//   then receive acknowledgements, executions, cancels, rejects, and state
//   updates for those same orders.
//
// Important OUCH 5.0 concepts represented here:
//   - inbound messages are client -> OUCH host
//   - outbound messages are OUCH host -> client
//   - outbound messages are sequenced by the lower-level session protocol
//   - inbound messages may be retransmitted benignly
//   - UserRefNum identifies an order transaction for the day on an OUCH port
//   - UserRefNum must be unique and strictly increasing
//   - timestamps are nanoseconds since midnight
//   - prices are unsigned numeric fields with 4 implied decimal places
//   - quantities must be greater than zero and less than 1,000,000
//
// Main inbound OUCH messages represented here:
//   O = Enter Order
//   U = Replace Order Request
//   X = Cancel Order Request
//   M = Modify Order Request
//   C = Mass Cancel Request
//   D = Disable Order Entry Request
//   E = Enable Order Entry Request
//   Q = Account Query Request
//
// Main outbound OUCH messages represented here:
//   S = System Event
//   A = Order Accepted
//   U = Order Replaced
//   C = Order Canceled
//   D = AIQ Canceled
//   E = Order Executed
//   B = Broken Trade
//   J = Rejected
//   P = Cancel Pending
//   I = Cancel Reject
//   T = Order Priority Update
//   M = Order Modified
//   R = Order Restated
//   X = Mass Cancel Response
//   G = Disable Order Entry Response
//   K = Enable Order Entry Response
//   Q = Account Query Response
//
// Future parser flow:
//
//   raw OUCH bytes
//   -> ouch_parser / ouch_encoder
//   -> these command/event structs
//   -> risk supervisor
//   -> lifecycle engine
//   -> execution backend / simulated exchange response

// -----------------------------------------------------------------------------
// OUCH aliases
// -----------------------------------------------------------------------------

using UserRefNum = std::uint32_t;
using UserRefIdx = std::uint8_t;
using OuchOrderRef = std::uint64_t;
using OuchPrice4 = std::uint64_t;
using OuchQuantity = std::uint32_t;
using OuchRejectCode = std::uint16_t;
using OuchMatchNumber = std::uint64_t;

using OuchSymbol = std::array<char, 8>;
using ClOrdId = std::array<char, 14>;
using Firm = std::array<char, 4>;

// Special OUCH market-order price value from the specification.
//
// OUCH prices have 4 implied decimal places.
// 2147483647 represents $214,748.3647.
inline constexpr OuchPrice4 ouch_market_order_price = 2147483647ULL;

inline constexpr OuchQuantity ouch_min_quantity = 1;
inline constexpr OuchQuantity ouch_max_quantity_exclusive = 1000000;

// -----------------------------------------------------------------------------
// OUCH message type enums
// -----------------------------------------------------------------------------

enum class OuchInboundMessageType : char {
    enter_order = 'O',
    replace_order = 'U',
    cancel_order = 'X',
    modify_order = 'M',
    mass_cancel = 'C',
    disable_order_entry = 'D',
    enable_order_entry = 'E',
    account_query = 'Q'
};

enum class OuchOutboundMessageType : char {
    system_event = 'S',
    order_accepted = 'A',
    order_replaced = 'U',
    order_canceled = 'C',
    aiq_canceled = 'D',
    order_executed = 'E',
    broken_trade = 'B',
    rejected = 'J',
    cancel_pending = 'P',
    cancel_reject = 'I',
    order_priority_update = 'T',
    order_modified = 'M',
    order_restated = 'R',
    mass_cancel_response = 'X',
    disable_order_entry_response = 'G',
    enable_order_entry_response = 'K',
    account_query_response = 'Q'
};

// -----------------------------------------------------------------------------
// OUCH field enums
// -----------------------------------------------------------------------------

enum class OuchSide : char {
    buy = 'B',
    sell = 'S',
    sell_short = 'T',
    sell_short_exempt = 'E'
};

enum class OuchTimeInForce : char {
    day = '0',
    immediate_or_cancel = '3',
    extended_hours = '5',
    good_till_time = '6',
    after_hours = 'E'
};

enum class OuchDisplay : char {
    visible = 'Y',
    hidden = 'N',
    attributable = 'A',
    conformant = 'Z'
};

enum class OuchCapacity : char {
    agency = 'A',
    principal = 'P',
    riskless = 'R',
    other = 'O'
};

enum class OuchIntermarketSweepEligibility : char {
    eligible = 'Y',
    not_eligible = 'N'
};

enum class OuchCrossType : char {
    continuous_market = 'N',
    opening_cross = 'O',
    closing_cross = 'C',
    halt_or_ipo = 'H',
    supplemental = 'S',
    retail = 'R',
    extended_life = 'E',
    after_hours_close = 'A'
};

enum class OuchOrderState : char {
    live = 'L',
    dead = 'D'
};

enum class OuchSystemEventCode : char {
    start_of_day = 'S',
    end_of_day = 'E'
};

// OUCH cancel reasons from the specification appendix.
enum class OuchCancelReason : char {
    regulatory_restriction = 'D',
    closed = 'E',
    post_only_cancel_nms = 'F',
    post_only_cancel_contra = 'G',
    halted = 'H',
    immediate_or_cancel = 'I',
    market_collars = 'K',
    self_match_prevention = 'Q',
    supervisory = 'S',
    timeout = 'T',
    user_requested = 'U',
    open_protection = 'X',
    system_cancel = 'Z',
    direct_listing_capital_raise_exceeds_allowed_shares = 'e'
};

// Small project-level liquidity classification.
// The raw OUCH liquidity flag is preserved as char on execution messages because
// the official appendix contains many market-specific single-character values.
enum class LiquidityFlag : std::uint8_t {
    unknown,
    added,
    removed
};

// Internal lifecycle status used by the simulator.
enum class OrderStatus : std::uint8_t {
    pending_new,
    accepted,
    partially_filled,
    filled,
    cancel_pending,
    canceled,
    replaced,
    rejected,
    modified,
    restated
};

// -----------------------------------------------------------------------------
// Inbound commands: client -> OUCH host
// -----------------------------------------------------------------------------

struct EnterOrderCommand {
    UserRefNum user_ref_num{};

    OuchSide side{OuchSide::buy};
    OuchQuantity quantity{};
    OuchSymbol symbol{};
    OuchPrice4 price{};

    OuchTimeInForce time_in_force{OuchTimeInForce::day};
    OuchDisplay display{OuchDisplay::visible};
    OuchCapacity capacity{OuchCapacity::agency};
    OuchIntermarketSweepEligibility intermarket_sweep_eligibility{
        OuchIntermarketSweepEligibility::not_eligible
    };
    OuchCrossType cross_type{OuchCrossType::continuous_market};

    ClOrdId cl_ord_id{};
    std::uint16_t appendage_length{};
};

struct ReplaceOrderCommand {
    UserRefNum original_user_ref_num{};
    UserRefNum replacement_user_ref_num{};

    OuchQuantity quantity{};
    OuchPrice4 price{};

    OuchTimeInForce time_in_force{OuchTimeInForce::day};
    OuchDisplay display{OuchDisplay::visible};
    OuchIntermarketSweepEligibility intermarket_sweep_eligibility{
        OuchIntermarketSweepEligibility::not_eligible
    };

    ClOrdId cl_ord_id{};
    std::uint16_t appendage_length{};
};

struct CancelOrderCommand {
    UserRefNum user_ref_num{};

    // OUCH cancel quantity is the new intended total order size.
    // Quantity 0 cancels the entire remaining balance.
    OuchQuantity intended_quantity{};
    std::uint16_t appendage_length{};
};

struct ModifyOrderCommand {
    UserRefNum user_ref_num{};
    OuchSide side{OuchSide::buy};

    // OUCH modify quantity is the new intended total order size.
    OuchQuantity intended_quantity{};
    std::uint16_t appendage_length{};
};

struct MassCancelCommand {
    UserRefNum user_ref_num{};
    Firm firm{};
    OuchSymbol symbol{};
    std::uint16_t appendage_length{};
};

struct DisableOrderEntryCommand {
    UserRefNum user_ref_num{};
    Firm firm{};
    std::uint16_t appendage_length{};
};

struct EnableOrderEntryCommand {
    UserRefNum user_ref_num{};
    Firm firm{};
    std::uint16_t appendage_length{};
};

struct AccountQueryCommand {
    std::uint16_t appendage_length{};
};

using OrderCommand = std::variant<
    EnterOrderCommand,
    ReplaceOrderCommand,
    CancelOrderCommand,
    ModifyOrderCommand,
    MassCancelCommand,
    DisableOrderEntryCommand,
    EnableOrderEntryCommand,
    AccountQueryCommand
>;

// -----------------------------------------------------------------------------
// Outbound events: OUCH host -> client
// -----------------------------------------------------------------------------

struct OuchSystemEvent {
    TimestampNs timestamp_ns{};
    OuchSystemEventCode event_code{OuchSystemEventCode::start_of_day};
};

struct OrderAcceptedEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    OuchSide side{OuchSide::buy};
    OuchQuantity quantity{};
    OuchSymbol symbol{};
    OuchPrice4 price{};

    OuchTimeInForce time_in_force{OuchTimeInForce::day};
    OuchDisplay display{OuchDisplay::visible};
    OuchOrderRef order_reference_number{};
    OuchCapacity capacity{OuchCapacity::agency};
    OuchIntermarketSweepEligibility intermarket_sweep_eligibility{
        OuchIntermarketSweepEligibility::not_eligible
    };
    OuchCrossType cross_type{OuchCrossType::continuous_market};
    OuchOrderState order_state{OuchOrderState::live};
    ClOrdId cl_ord_id{};
    std::uint16_t appendage_length{};
};

struct OrderReplacedEvent {
    TimestampNs timestamp_ns{};
    UserRefNum original_user_ref_num{};
    UserRefNum replacement_user_ref_num{};

    OuchSide side{OuchSide::buy};
    OuchQuantity quantity{};
    OuchSymbol symbol{};
    OuchPrice4 price{};

    OuchTimeInForce time_in_force{OuchTimeInForce::day};
    OuchDisplay display{OuchDisplay::visible};
    OuchOrderRef order_reference_number{};
    OuchCapacity capacity{OuchCapacity::agency};
    OuchIntermarketSweepEligibility intermarket_sweep_eligibility{
        OuchIntermarketSweepEligibility::not_eligible
    };
    OuchCrossType cross_type{OuchCrossType::continuous_market};
    OuchOrderState order_state{OuchOrderState::live};
    ClOrdId cl_ord_id{};
    std::uint16_t appendage_length{};
};

struct OrderCanceledEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    // Incremental shares decremented from the order.
    OuchQuantity canceled_quantity{};
    OuchCancelReason reason{OuchCancelReason::user_requested};
    std::uint16_t appendage_length{};
};

struct AiqCanceledEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    OuchQuantity decrement_shares{};
    OuchCancelReason reason{OuchCancelReason::self_match_prevention};
    OuchQuantity quantity_prevented_from_trading{};
    OuchPrice4 execution_price{};
    char raw_liquidity_flag{' '};
    char aiq_strategy{' '};
    std::uint16_t appendage_length{};
};

struct OrderExecutedEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    // Incremental shares executed in this execution.
    OuchQuantity executed_quantity{};
    OuchPrice4 execution_price{};
    char raw_liquidity_flag{' '};
    OuchMatchNumber match_number{};
    std::uint16_t appendage_length{};
};

struct BrokenTradeEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    OuchMatchNumber match_number{};
    char reason{' '};
    ClOrdId cl_ord_id{};
    std::uint16_t appendage_length{};
};

struct OrderRejectedEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    OuchRejectCode reason{};
    ClOrdId cl_ord_id{};
    std::uint16_t appendage_length{};
};

struct CancelPendingEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};
    std::uint16_t appendage_length{};
};

struct CancelRejectEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};
    std::uint16_t appendage_length{};
};

struct OrderPriorityUpdateEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    OuchPrice4 price{};
    OuchDisplay display{OuchDisplay::visible};
    OuchOrderRef order_reference_number{};
    std::uint16_t appendage_length{};
};

struct OrderModifiedEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    OuchSide side{OuchSide::buy};
    OuchQuantity quantity{};
    std::uint16_t appendage_length{};
};

struct OrderRestatedEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    char reason{' '};
    std::uint16_t appendage_length{};
};

struct MassCancelResponseEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    Firm firm{};
    OuchSymbol symbol{};
    std::uint16_t appendage_length{};
};

struct DisableOrderEntryResponseEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    Firm firm{};
    std::uint16_t appendage_length{};
};

struct EnableOrderEntryResponseEvent {
    TimestampNs timestamp_ns{};
    UserRefNum user_ref_num{};

    Firm firm{};
    std::uint16_t appendage_length{};
};

struct AccountQueryResponseEvent {
    TimestampNs timestamp_ns{};
    UserRefNum next_user_ref_num{};
    std::uint16_t appendage_length{};
};

using OrderEvent = std::variant<
    OuchSystemEvent,
    OrderAcceptedEvent,
    OrderReplacedEvent,
    OrderCanceledEvent,
    AiqCanceledEvent,
    OrderExecutedEvent,
    BrokenTradeEvent,
    OrderRejectedEvent,
    CancelPendingEvent,
    CancelRejectEvent,
    OrderPriorityUpdateEvent,
    OrderModifiedEvent,
    OrderRestatedEvent,
    MassCancelResponseEvent,
    DisableOrderEntryResponseEvent,
    EnableOrderEntryResponseEvent,
    AccountQueryResponseEvent
>;

// -----------------------------------------------------------------------------
// Conversion helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr Side to_book_side(OuchSide side) noexcept {
    switch (side) {
        case OuchSide::buy:
            return Side::bid;
        case OuchSide::sell:
        case OuchSide::sell_short:
        case OuchSide::sell_short_exempt:
            return Side::ask;
    }

    return Side::bid;
}

[[nodiscard]] constexpr LiquidityFlag classify_liquidity_flag(
    char raw_liquidity_flag
) noexcept {
    switch (raw_liquidity_flag) {
        case 'A':
        case 'J':
        case 'k':
        case 'u':
        case '7':
        case '8':
        case '1':
        case '2':
        case '5':
        case '9':
            return LiquidityFlag::added;

        case 'R':
        case 'm':
        case 'p':
        case 'q':
        case 'r':
        case 't':
        case '3':
            return LiquidityFlag::removed;

        default:
            return LiquidityFlag::unknown;
    }
}

// -----------------------------------------------------------------------------
// Validation helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr bool is_valid_user_ref_num(UserRefNum user_ref_num) noexcept {
    return user_ref_num > 0;
}

[[nodiscard]] constexpr bool is_valid_ouch_quantity(
    OuchQuantity quantity
) noexcept {
    return quantity >= ouch_min_quantity
        && quantity < ouch_max_quantity_exclusive;
}

[[nodiscard]] constexpr bool is_valid_cancel_intended_quantity(
    OuchQuantity intended_quantity
) noexcept {
    return intended_quantity < ouch_max_quantity_exclusive;
}

[[nodiscard]] constexpr bool is_valid_ouch_price(OuchPrice4 price) noexcept {
    return price > 0;
}

[[nodiscard]] constexpr bool is_market_order_price(OuchPrice4 price) noexcept {
    return price == ouch_market_order_price;
}

[[nodiscard]] constexpr bool is_valid_enter_order(
    const EnterOrderCommand& command
) noexcept {
    return is_valid_user_ref_num(command.user_ref_num)
        && is_valid_ouch_quantity(command.quantity)
        && is_valid_ouch_price(command.price);
}

[[nodiscard]] constexpr bool is_valid_replace_order(
    const ReplaceOrderCommand& command
) noexcept {
    return is_valid_user_ref_num(command.original_user_ref_num)
        && is_valid_user_ref_num(command.replacement_user_ref_num)
        && command.original_user_ref_num != command.replacement_user_ref_num
        && is_valid_ouch_quantity(command.quantity)
        && is_valid_ouch_price(command.price);
}

[[nodiscard]] constexpr bool is_valid_cancel_order(
    const CancelOrderCommand& command
) noexcept {
    return is_valid_user_ref_num(command.user_ref_num)
        && is_valid_cancel_intended_quantity(command.intended_quantity);
}

[[nodiscard]] constexpr bool is_valid_modify_order(
    const ModifyOrderCommand& command
) noexcept {
    return is_valid_user_ref_num(command.user_ref_num)
        && is_valid_cancel_intended_quantity(command.intended_quantity);
}

[[nodiscard]] constexpr bool is_valid_mass_cancel(
    const MassCancelCommand& command
) noexcept {
    return is_valid_user_ref_num(command.user_ref_num);
}

[[nodiscard]] constexpr bool is_valid_disable_order_entry(
    const DisableOrderEntryCommand& command
) noexcept {
    return is_valid_user_ref_num(command.user_ref_num);
}

[[nodiscard]] constexpr bool is_valid_enable_order_entry(
    const EnableOrderEntryCommand& command
) noexcept {
    return is_valid_user_ref_num(command.user_ref_num);
}

[[nodiscard]] constexpr bool is_terminal_status(OrderStatus status) noexcept {
    return status == OrderStatus::filled
        || status == OrderStatus::canceled
        || status == OrderStatus::replaced
        || status == OrderStatus::rejected;
}

[[nodiscard]] constexpr bool is_live_status(OrderStatus status) noexcept {
    return status == OrderStatus::accepted
        || status == OrderStatus::partially_filled
        || status == OrderStatus::modified
        || status == OrderStatus::restated;
}

// -----------------------------------------------------------------------------
// String helpers
// -----------------------------------------------------------------------------

[[nodiscard]] constexpr std::string_view to_string(
    OuchInboundMessageType message_type
) noexcept {
    switch (message_type) {
        case OuchInboundMessageType::enter_order:
            return "enter_order";
        case OuchInboundMessageType::replace_order:
            return "replace_order";
        case OuchInboundMessageType::cancel_order:
            return "cancel_order";
        case OuchInboundMessageType::modify_order:
            return "modify_order";
        case OuchInboundMessageType::mass_cancel:
            return "mass_cancel";
        case OuchInboundMessageType::disable_order_entry:
            return "disable_order_entry";
        case OuchInboundMessageType::enable_order_entry:
            return "enable_order_entry";
        case OuchInboundMessageType::account_query:
            return "account_query";
    }

    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(
    OuchOutboundMessageType message_type
) noexcept {
    switch (message_type) {
        case OuchOutboundMessageType::system_event:
            return "system_event";
        case OuchOutboundMessageType::order_accepted:
            return "order_accepted";
        case OuchOutboundMessageType::order_replaced:
            return "order_replaced";
        case OuchOutboundMessageType::order_canceled:
            return "order_canceled";
        case OuchOutboundMessageType::aiq_canceled:
            return "aiq_canceled";
        case OuchOutboundMessageType::order_executed:
            return "order_executed";
        case OuchOutboundMessageType::broken_trade:
            return "broken_trade";
        case OuchOutboundMessageType::rejected:
            return "rejected";
        case OuchOutboundMessageType::cancel_pending:
            return "cancel_pending";
        case OuchOutboundMessageType::cancel_reject:
            return "cancel_reject";
        case OuchOutboundMessageType::order_priority_update:
            return "order_priority_update";
        case OuchOutboundMessageType::order_modified:
            return "order_modified";
        case OuchOutboundMessageType::order_restated:
            return "order_restated";
        case OuchOutboundMessageType::mass_cancel_response:
            return "mass_cancel_response";
        case OuchOutboundMessageType::disable_order_entry_response:
            return "disable_order_entry_response";
        case OuchOutboundMessageType::enable_order_entry_response:
            return "enable_order_entry_response";
        case OuchOutboundMessageType::account_query_response:
            return "account_query_response";
    }

    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(OrderStatus status) noexcept {
    switch (status) {
        case OrderStatus::pending_new:
            return "pending_new";
        case OrderStatus::accepted:
            return "accepted";
        case OrderStatus::partially_filled:
            return "partially_filled";
        case OrderStatus::filled:
            return "filled";
        case OrderStatus::cancel_pending:
            return "cancel_pending";
        case OrderStatus::canceled:
            return "canceled";
        case OrderStatus::replaced:
            return "replaced";
        case OrderStatus::rejected:
            return "rejected";
        case OrderStatus::modified:
            return "modified";
        case OrderStatus::restated:
            return "restated";
    }

    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(
    LiquidityFlag liquidity_flag
) noexcept {
    switch (liquidity_flag) {
        case LiquidityFlag::unknown:
            return "unknown";
        case LiquidityFlag::added:
            return "added";
        case LiquidityFlag::removed:
            return "removed";
    }

    return "unknown";
}

} // namespace fgep