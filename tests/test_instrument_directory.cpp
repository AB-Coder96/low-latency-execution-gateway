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

[[nodiscard]] fgep::itch::ReasonCode4 reason_code(const char* text) {
    const auto result = fgep::wire::make_fixed_ascii<4>(text);
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

[[nodiscard]] fgep::itch::StockTradingActionMessage trading_action(
    fgep::itch::StockLocate stock_locate,
    const char* symbol,
    fgep::itch::TradingState trading_state
) {
    fgep::itch::StockTradingActionMessage message{};

    message.header.stock_locate = stock_locate;
    message.header.tracking_number = 2;
    message.header.timestamp_ns = 200;

    message.stock = stock_symbol(symbol);
    message.trading_state = trading_state;
    message.reserved = ' ';
    message.reason = reason_code("    ");

    return message;
}

} // namespace

int main() {
    using namespace fgep;

    instrument::InstrumentDirectory directory{};

    assert(directory.size() == 0);

    assert(
        directory.apply(venue::VenueId{1}, stock_directory(1, "AAPL"))
        == ErrorCode::ok
    );

    assert(directory.size() == 1);

    const auto* aapl = directory.find(venue::VenueId{1}, itch::StockLocate{1});

    assert(aapl != nullptr);
    assert(aapl->key.venue_id == 1);
    assert(aapl->key.stock_locate == 1);
    assert(wire::fixed_ascii_equals(aapl->stock, "AAPL"));
    assert(aapl->round_lot_size == 100);
    assert(aapl->trading_status == instrument::SymbolTradingStatus::unknown);

    assert(
        directory.apply(
            venue::VenueId{1},
            trading_action(1, "AAPL", itch::TradingState::trading)
        ) == ErrorCode::ok
    );

    aapl = directory.find(venue::VenueId{1}, itch::StockLocate{1});

    assert(aapl != nullptr);
    assert(aapl->trading_status == instrument::SymbolTradingStatus::trading);

    assert(
        directory.apply(venue::VenueId{2}, stock_directory(1, "AAPL"))
        == ErrorCode::ok
    );

    assert(directory.size() == 2);

    const instrument::CanonicalSymbol canonical_aapl{
        stock_symbol("AAPL")
    };

    const auto aapl_instances =
        directory.instruments_for_symbol(canonical_aapl);

    assert(aapl_instances.size() == 2);

    assert(
        directory.apply(
            venue::VenueId{1},
            trading_action(99, "MSFT", itch::TradingState::halted)
        ) == ErrorCode::not_found
    );

    assert(
        directory.apply(venue::VenueId{1}, stock_directory(2, "MSFT"))
        == ErrorCode::ok
    );

    assert(
        directory.apply(
            venue::VenueId{1},
            trading_action(2, "MSFT", itch::TradingState::halted)
        ) == ErrorCode::ok
    );

    const auto* msft = directory.find(venue::VenueId{1}, itch::StockLocate{2});

    assert(msft != nullptr);
    assert(msft->trading_status == instrument::SymbolTradingStatus::halted);

    return 0;
}