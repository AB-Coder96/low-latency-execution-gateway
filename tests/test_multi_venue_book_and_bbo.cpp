#include "fgep/bbo/normalized_bbo.hpp"
#include "fgep/book/multi_venue_book.hpp"
#include "fgep/instrument/instrument_directory.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cassert>

namespace {

[[nodiscard]] fgep::itch::StockSymbol stock_symbol(const char* text) {
    const auto result = fgep::wire::make_fixed_ascii<8>(text);
    assert(result.ok());
    return result.value;
}

[[nodiscard]] fgep::itch::IssueSubType issue_sub_type(const char* text) {
    const auto result = fgep::wire::make_fixed_ascii<2>(text);
    assert(result.ok());
    return result.value;
}

[[nodiscard]] fgep::itch::StockDirectoryMessage stock_directory(
    fgep::itch::StockLocate stock_locate,
    const char* symbol
) {
    fgep::itch::StockDirectoryMessage message{};

    message.header.stock_locate = stock_locate;
    message.header.tracking_number = 1;
    message.header.timestamp_ns = 100;

    message.stock = stock_symbol(symbol);
    message.market_category = 'Q';
    message.financial_status_indicator = 'N';
    message.round_lot_size = 100;
    message.round_lots_only = 'N';
    message.issue_classification = 'C';
    message.issue_sub_type = issue_sub_type("  ");
    message.authenticity = 'P';
    message.short_sale_threshold_indicator = 'N';
    message.ipo_flag = 'N';
    message.luld_reference_price_tier = '1';
    message.etp_flag = 'N';
    message.etp_leverage_factor = 0;
    message.inverse_indicator = 'N';

    return message;
}

[[nodiscard]] fgep::itch::AddOrderNoMpidMessage add_order(
    fgep::itch::StockLocate stock_locate,
    fgep::itch::OrderReferenceNumber order_reference_number,
    fgep::itch::Side side,
    fgep::itch::Shares shares,
    const char* symbol,
    fgep::itch::Price4 price
) {
    fgep::itch::AddOrderNoMpidMessage message{};

    message.header.stock_locate = stock_locate;
    message.header.tracking_number = 10;
    message.header.timestamp_ns = 1000;

    message.order_reference_number = order_reference_number;
    message.side = side;
    message.shares = shares;
    message.stock = stock_symbol(symbol);
    message.price = price;

    return message;
}

[[nodiscard]] fgep::itch::OrderCancelMessage cancel_order(
    fgep::itch::StockLocate stock_locate,
    fgep::itch::OrderReferenceNumber order_reference_number,
    fgep::itch::Shares shares
) {
    fgep::itch::OrderCancelMessage message{};

    message.header.stock_locate = stock_locate;
    message.header.tracking_number = 20;
    message.header.timestamp_ns = 2000;

    message.order_reference_number = order_reference_number;
    message.cancelled_shares = shares;

    return message;
}

} // namespace

int main() {
    using namespace fgep;

    instrument::InstrumentDirectory directory{};

    assert(
        directory.apply(venue::VenueId{1}, stock_directory(1, "AAPL"))
        == ErrorCode::ok
    );

    assert(
        directory.apply(venue::VenueId{2}, stock_directory(7, "AAPL"))
        == ErrorCode::ok
    );

    assert(
        directory.apply(venue::VenueId{1}, stock_directory(2, "MSFT"))
        == ErrorCode::ok
    );

    book::MultiVenueBook books{directory};

    assert(books.add_venue(venue::VenueId{1}) == ErrorCode::ok);
    assert(books.add_venue(venue::VenueId{2}) == ErrorCode::ok);
    assert(books.add_venue(venue::VenueId{2}) == ErrorCode::duplicate);
    assert(books.venue_count() == 2);

    const instrument::CanonicalSymbol aapl{
        stock_symbol("AAPL")
    };

    const instrument::CanonicalSymbol msft{
        stock_symbol("MSFT")
    };

    assert(
        books.apply(
            venue::VenueId{1},
            itch::Message{
                add_order(1, 100, itch::Side::buy, 100, "AAPL", 1902500)
            }
        ) == ErrorCode::ok
    );

    assert(
        books.apply(
            venue::VenueId{1},
            itch::Message{
                add_order(1, 101, itch::Side::sell, 80, "AAPL", 1904000)
            }
        ) == ErrorCode::ok
    );

    assert(
        books.apply(
            venue::VenueId{2},
            itch::Message{
                add_order(7, 200, itch::Side::buy, 50, "AAPL", 1902600)
            }
        ) == ErrorCode::ok
    );

    assert(
        books.apply(
            venue::VenueId{2},
            itch::Message{
                add_order(7, 201, itch::Side::sell, 70, "AAPL", 1903500)
            }
        ) == ErrorCode::ok
    );

    bbo::NormalizedBboView bbo_view{books};

    auto aapl_bbo = bbo_view.get(aapl);

    assert(aapl_bbo.bid.has_value());
    assert(aapl_bbo.bid->key.venue_id == 2);
    assert(aapl_bbo.bid->key.stock_locate == 7);
    assert(aapl_bbo.bid->price == 1902600);
    assert(aapl_bbo.bid->quantity == 50);

    assert(aapl_bbo.ask.has_value());
    assert(aapl_bbo.ask->key.venue_id == 2);
    assert(aapl_bbo.ask->key.stock_locate == 7);
    assert(aapl_bbo.ask->price == 1903500);
    assert(aapl_bbo.ask->quantity == 70);

    assert(
        books.apply(
            venue::VenueId{1},
            itch::Message{
                add_order(1, 102, itch::Side::sell, 25, "AAPL", 1903400)
            }
        ) == ErrorCode::ok
    );

    aapl_bbo = bbo_view.get(aapl);

    assert(aapl_bbo.ask.has_value());
    assert(aapl_bbo.ask->key.venue_id == 1);
    assert(aapl_bbo.ask->price == 1903400);
    assert(aapl_bbo.ask->quantity == 25);

    assert(
        books.apply(
            venue::VenueId{1},
            itch::Message{
                cancel_order(1, 102, 25)
            }
        ) == ErrorCode::ok
    );

    aapl_bbo = bbo_view.get(aapl);

    assert(aapl_bbo.ask.has_value());
    assert(aapl_bbo.ask->key.venue_id == 2);
    assert(aapl_bbo.ask->price == 1903500);

    assert(
        books.apply(
            venue::VenueId{1},
            itch::Message{
                add_order(2, 300, itch::Side::buy, 40, "MSFT", 3300000)
            }
        ) == ErrorCode::ok
    );

    auto msft_bbo = bbo_view.get(msft);

    assert(msft_bbo.bid.has_value());
    assert(msft_bbo.bid->key.venue_id == 1);
    assert(msft_bbo.bid->key.stock_locate == 2);
    assert(msft_bbo.bid->price == 3300000);

    msft_bbo = bbo_view.get(msft);

    assert(!msft_bbo.ask.has_value());

    assert(
        books.apply(
            venue::VenueId{2},
            itch::Message{
                add_order(99, 400, itch::Side::buy, 10, "TSLA", 2500000)
            }
        ) == ErrorCode::not_found
    );

    assert(
        books.apply(
            venue::VenueId{9},
            itch::Message{
                add_order(1, 500, itch::Side::buy, 10, "AAPL", 1900000)
            }
        ) == ErrorCode::not_found
    );

    return 0;
}