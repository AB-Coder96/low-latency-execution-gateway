#include "fgep/core/market_event.hpp"

#include <cassert>
#include <string_view>
#include <variant>

int main() {
    using namespace fgep;

    const ItchHeader valid_header{
        .stock_locate = 1,
        .tracking_number = 7,
        .timestamp_ns = 1000
    };

    // ITCH side converts into book side.
    assert(to_book_side(ItchSide::buy) == Side::bid);
    assert(to_book_side(ItchSide::sell) == Side::ask);

    // Header and primitive validation.
    assert(is_valid_stock_locate(1));
    assert(!is_valid_stock_locate(0));

    assert(is_valid_itch_header(valid_header));

    const ItchHeader invalid_header{
        .stock_locate = 1,
        .tracking_number = 7,
        .timestamp_ns = 0
    };

    assert(!is_valid_itch_header(invalid_header));

    assert(is_valid_price4(1));
    assert(!is_valid_price4(0));

    // Message type helpers.
    assert(is_add_order(ItchMessageType::add_order_no_mpid));
    assert(is_add_order(ItchMessageType::add_order_with_mpid));
    assert(!is_add_order(ItchMessageType::order_delete));

    assert(is_visible_book_update(ItchMessageType::add_order_no_mpid));
    assert(is_visible_book_update(ItchMessageType::add_order_with_mpid));
    assert(is_visible_book_update(ItchMessageType::order_executed));
    assert(is_visible_book_update(ItchMessageType::order_executed_with_price));
    assert(is_visible_book_update(ItchMessageType::order_cancel));
    assert(is_visible_book_update(ItchMessageType::order_delete));
    assert(is_visible_book_update(ItchMessageType::order_replace));
    assert(!is_visible_book_update(ItchMessageType::trade_non_cross));
    assert(!is_visible_book_update(ItchMessageType::cross_trade));
    assert(!is_visible_book_update(ItchMessageType::broken_trade));

    // Add order without MPID affects the visible book when valid.
    const AddOrderNoMpidMessage add_no_mpid{
        .header = valid_header,
        .order_reference_number = 100,
        .side = ItchSide::buy,
        .shares = 250,
        .stock = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
        .price = 1902500
    };

    assert(affects_visible_book(add_no_mpid));

    const AddOrderNoMpidMessage bad_add_no_mpid{
        .header = valid_header,
        .order_reference_number = 0,
        .side = ItchSide::buy,
        .shares = 250,
        .stock = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
        .price = 1902500
    };

    assert(!affects_visible_book(bad_add_no_mpid));

    // Add order with MPID affects the visible book when valid.
    const AddOrderWithMpidMessage add_with_mpid{
        .header = valid_header,
        .order_reference_number = 101,
        .side = ItchSide::sell,
        .shares = 100,
        .stock = {'M', 'S', 'F', 'T', ' ', ' ', ' ', ' '},
        .price = 4201250,
        .attribution = {'N', 'S', 'D', 'Q'}
    };

    assert(affects_visible_book(add_with_mpid));

    // Executions and cancel/delete/replace update the visible book.
    const OrderExecutedMessage executed{
        .header = valid_header,
        .order_reference_number = 100,
        .executed_shares = 50,
        .match_number = 9001
    };

    assert(affects_visible_book(executed));

    const OrderExecutedWithPriceMessage executed_with_price{
        .header = valid_header,
        .order_reference_number = 100,
        .executed_shares = 25,
        .match_number = 9002,
        .printable = PrintableFlag::printable,
        .execution_price = 1902000
    };

    assert(affects_visible_book(executed_with_price));

    const OrderCancelMessage cancel{
        .header = valid_header,
        .order_reference_number = 100,
        .cancelled_shares = 10
    };

    assert(affects_visible_book(cancel));

    const OrderDeleteMessage delete_order{
        .header = valid_header,
        .order_reference_number = 100
    };

    assert(affects_visible_book(delete_order));

    const OrderReplaceMessage replace{
        .header = valid_header,
        .original_order_reference_number = 100,
        .new_order_reference_number = 200,
        .shares = 300,
        .price = 1903000
    };

    assert(affects_visible_book(replace));

    // Trade and break messages do not directly update the visible book model.
    const TradeNonCrossMessage trade{
        .header = valid_header,
        .order_reference_number = 0,
        .side = ItchSide::buy,
        .shares = 100,
        .stock = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
        .price = 1902600,
        .match_number = 9003
    };

    assert(!affects_visible_book(trade));

    const CrossTradeMessage cross_trade{
        .header = valid_header,
        .shares = 100000,
        .stock = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '},
        .cross_price = 1900000,
        .match_number = 9004,
        .cross_type = CrossType::opening_cross
    };

    assert(!affects_visible_book(cross_trade));

    const BrokenTradeMessage broken_trade{
        .header = valid_header,
        .match_number = 9003
    };

    assert(!affects_visible_book(broken_trade));

    // Variant can hold supported ITCH messages.
    MarketEvent event = add_no_mpid;
    assert(std::holds_alternative<AddOrderNoMpidMessage>(event));

    event = delete_order;
    assert(std::holds_alternative<OrderDeleteMessage>(event));

    // String conversion.
    assert(to_string(ItchMessageType::add_order_no_mpid) == std::string_view{"add_order_no_mpid"});
    assert(to_string(ItchMessageType::order_replace) == std::string_view{"order_replace"});
    assert(to_string(ItchMessageType::trade_non_cross) == std::string_view{"trade_non_cross"});

    return 0;
}