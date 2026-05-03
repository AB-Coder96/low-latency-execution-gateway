#include "fgep/itch/itch_message_type.hpp"

#include <cassert>
#include <string_view>

int main() {
    using namespace fgep;
    using namespace fgep::itch;

    // -------------------------------------------------------------------------
    // Character conversion: supported message types
    // -------------------------------------------------------------------------

    {
        const auto result = message_type_from_char('S');

        assert(result.ok());
        assert(result.value == MessageType::system_event);
        assert(to_char(result.value) == 'S');
    }

    {
        const auto result = message_type_from_char('A');

        assert(result.ok());
        assert(result.value == MessageType::add_order_no_mpid);
        assert(to_char(result.value) == 'A');
    }

    {
        const auto result = message_type_from_char('F');

        assert(result.ok());
        assert(result.value == MessageType::add_order_with_mpid);
        assert(to_char(result.value) == 'F');
    }

    {
        const auto result = message_type_from_char('E');

        assert(result.ok());
        assert(result.value == MessageType::order_executed);
        assert(to_char(result.value) == 'E');
    }

    {
        const auto result = message_type_from_char('C');

        assert(result.ok());
        assert(result.value == MessageType::order_executed_with_price);
        assert(to_char(result.value) == 'C');
    }

    {
        const auto result = message_type_from_char('X');

        assert(result.ok());
        assert(result.value == MessageType::order_cancel);
        assert(to_char(result.value) == 'X');
    }

    {
        const auto result = message_type_from_char('D');

        assert(result.ok());
        assert(result.value == MessageType::order_delete);
        assert(to_char(result.value) == 'D');
    }

    {
        const auto result = message_type_from_char('U');

        assert(result.ok());
        assert(result.value == MessageType::order_replace);
        assert(to_char(result.value) == 'U');
    }

    {
        const auto result = message_type_from_char('P');

        assert(result.ok());
        assert(result.value == MessageType::trade_non_cross);
        assert(to_char(result.value) == 'P');
    }

    {
        const auto result = message_type_from_char('Q');

        assert(result.ok());
        assert(result.value == MessageType::cross_trade);
        assert(to_char(result.value) == 'Q');
    }

    {
        const auto result = message_type_from_char('B');

        assert(result.ok());
        assert(result.value == MessageType::broken_trade);
        assert(to_char(result.value) == 'B');
    }

    // -------------------------------------------------------------------------
    // Character conversion: known administrative message types
    // -------------------------------------------------------------------------

    assert(message_type_from_char('R').value == MessageType::stock_directory);
    assert(message_type_from_char('H').value == MessageType::stock_trading_action);
    assert(message_type_from_char('Y').value == MessageType::reg_sho_restriction);
    assert(message_type_from_char('L').value == MessageType::market_participant_position);
    assert(message_type_from_char('V').value == MessageType::mwcb_decline_level);
    assert(message_type_from_char('W').value == MessageType::mwcb_status);
    assert(message_type_from_char('K').value == MessageType::ipo_quoting_period_update);
    assert(message_type_from_char('J').value == MessageType::luld_auction_collar);
    assert(message_type_from_char('h').value == MessageType::operational_halt);
    assert(message_type_from_char('I').value == MessageType::noii);
    assert(message_type_from_char('N').value == MessageType::retail_price_improvement_indicator);
    assert(message_type_from_char('O').value == MessageType::direct_listing_with_capital_raise);

    // -------------------------------------------------------------------------
    // Unknown message type
    // -------------------------------------------------------------------------

    {
        const auto result = message_type_from_char('?');

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    assert(is_known_message_type('A'));
    assert(is_known_message_type('S'));
    assert(!is_known_message_type('?'));

    // -------------------------------------------------------------------------
    // String conversion
    // -------------------------------------------------------------------------

    assert(to_string(MessageType::system_event) == std::string_view{"system_event"});
    assert(to_string(MessageType::add_order_no_mpid) == std::string_view{"add_order_no_mpid"});
    assert(to_string(MessageType::add_order_with_mpid) == std::string_view{"add_order_with_mpid"});
    assert(to_string(MessageType::order_executed) == std::string_view{"order_executed"});
    assert(
        to_string(MessageType::order_executed_with_price)
        == std::string_view{"order_executed_with_price"}
    );
    assert(to_string(MessageType::order_cancel) == std::string_view{"order_cancel"});
    assert(to_string(MessageType::order_delete) == std::string_view{"order_delete"});
    assert(to_string(MessageType::order_replace) == std::string_view{"order_replace"});
    assert(to_string(MessageType::trade_non_cross) == std::string_view{"trade_non_cross"});
    assert(to_string(MessageType::cross_trade) == std::string_view{"cross_trade"});
    assert(to_string(MessageType::broken_trade) == std::string_view{"broken_trade"});

    // -------------------------------------------------------------------------
    // Category helpers
    // -------------------------------------------------------------------------

    assert(is_administrative_message(MessageType::system_event));
    assert(is_administrative_message(MessageType::stock_directory));
    assert(is_administrative_message(MessageType::stock_trading_action));
    assert(is_administrative_message(MessageType::operational_halt));
    assert(!is_administrative_message(MessageType::add_order_no_mpid));
    assert(!is_administrative_message(MessageType::order_executed));
    assert(!is_administrative_message(MessageType::trade_non_cross));

    assert(is_add_order_message(MessageType::add_order_no_mpid));
    assert(is_add_order_message(MessageType::add_order_with_mpid));
    assert(!is_add_order_message(MessageType::order_executed));

    assert(is_order_book_modify_message(MessageType::order_executed));
    assert(is_order_book_modify_message(MessageType::order_executed_with_price));
    assert(is_order_book_modify_message(MessageType::order_cancel));
    assert(is_order_book_modify_message(MessageType::order_delete));
    assert(is_order_book_modify_message(MessageType::order_replace));
    assert(!is_order_book_modify_message(MessageType::add_order_no_mpid));

    assert(is_visible_book_update_message(MessageType::add_order_no_mpid));
    assert(is_visible_book_update_message(MessageType::add_order_with_mpid));
    assert(is_visible_book_update_message(MessageType::order_executed));
    assert(is_visible_book_update_message(MessageType::order_cancel));
    assert(is_visible_book_update_message(MessageType::order_delete));
    assert(is_visible_book_update_message(MessageType::order_replace));
    assert(!is_visible_book_update_message(MessageType::trade_non_cross));
    assert(!is_visible_book_update_message(MessageType::cross_trade));
    assert(!is_visible_book_update_message(MessageType::broken_trade));

    assert(is_trade_message(MessageType::trade_non_cross));
    assert(is_trade_message(MessageType::cross_trade));
    assert(is_trade_message(MessageType::broken_trade));
    assert(!is_trade_message(MessageType::add_order_no_mpid));

    // -------------------------------------------------------------------------
    // Exact supported message lengths
    // -------------------------------------------------------------------------

    {
        const auto result = message_length(MessageType::system_event);

        assert(result.ok());
        assert(result.value == length_system_event);
        assert(result.value == 12);
    }

    {
        const auto result = message_length(MessageType::add_order_no_mpid);

        assert(result.ok());
        assert(result.value == length_add_order_no_mpid);
        assert(result.value == 36);
    }

    {
        const auto result = message_length(MessageType::add_order_with_mpid);

        assert(result.ok());
        assert(result.value == length_add_order_with_mpid);
        assert(result.value == 40);
    }

    {
        const auto result = message_length(MessageType::order_executed);

        assert(result.ok());
        assert(result.value == length_order_executed);
        assert(result.value == 31);
    }

    {
        const auto result = message_length(MessageType::order_executed_with_price);

        assert(result.ok());
        assert(result.value == length_order_executed_with_price);
        assert(result.value == 36);
    }

    {
        const auto result = message_length(MessageType::order_cancel);

        assert(result.ok());
        assert(result.value == length_order_cancel);
        assert(result.value == 23);
    }

    {
        const auto result = message_length(MessageType::order_delete);

        assert(result.ok());
        assert(result.value == length_order_delete);
        assert(result.value == 19);
    }

    {
        const auto result = message_length(MessageType::order_replace);

        assert(result.ok());
        assert(result.value == length_order_replace);
        assert(result.value == 35);
    }

    {
        const auto result = message_length(MessageType::trade_non_cross);

        assert(result.ok());
        assert(result.value == length_trade_non_cross);
        assert(result.value == 44);
    }

    {
        const auto result = message_length(MessageType::cross_trade);

        assert(result.ok());
        assert(result.value == length_cross_trade);
        assert(result.value == 40);
    }

    {
        const auto result = message_length(MessageType::broken_trade);

        assert(result.ok());
        assert(result.value == length_broken_trade);
        assert(result.value == 19);
    }

    // -------------------------------------------------------------------------
    // Unsupported lengths for admin messages not implemented in first pass
    // -------------------------------------------------------------------------

    {
        const auto result = message_length(MessageType::stock_directory);

        assert(result.failed());
        assert(result.error == ErrorCode::unsupported);
        assert(result.value == 0);
    }

    {
        const auto result = message_length(MessageType::noii);

        assert(result.failed());
        assert(result.error == ErrorCode::unsupported);
        assert(result.value == 0);
    }

    assert(has_supported_length(MessageType::system_event));
    assert(has_supported_length(MessageType::add_order_no_mpid));
    assert(!has_supported_length(MessageType::stock_directory));
    assert(!has_supported_length(MessageType::noii));

    return 0;
}