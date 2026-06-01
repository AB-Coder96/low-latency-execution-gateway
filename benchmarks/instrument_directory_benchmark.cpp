#include "fgep/instrument/instrument_directory.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <benchmark/benchmark.h>

namespace {

[[nodiscard]] fgep::itch::StockSymbol stock_symbol(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<8>(text);

    if (value.failed()) {
        return {};
    }

    return value.value;
}

[[nodiscard]] fgep::itch::IssueSubType issue_sub_type(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<2>(text);

    if (value.failed()) {
        return {};
    }

    return value.value;
}

[[nodiscard]] fgep::itch::ReasonCode4 reason_code(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<4>(text);

    if (value.failed()) {
        return {};
    }

    return value.value;
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

void BM_InstrumentDirectoryApplyStockDirectory(benchmark::State& state) {
    fgep::instrument::InstrumentDirectory directory{};
    fgep::itch::StockLocate stock_locate = 1;

    for (auto _ : state) {
        const auto message = stock_directory(stock_locate++, "AAPL");

        const auto error = directory.apply(
            fgep::venue::VenueId{1},
            message
        );

        benchmark::DoNotOptimize(error);

        if (directory.size() >= 4096U) {
            state.PauseTiming();
            directory = fgep::instrument::InstrumentDirectory{};
            stock_locate = 1;
            state.ResumeTiming();
        }
    }
}

void BM_InstrumentDirectoryApplyTradingAction(benchmark::State& state) {
    fgep::itch::StockLocate stock_locate = 1;

    for (auto _ : state) {
        state.PauseTiming();

        fgep::instrument::InstrumentDirectory directory{};
        benchmark::DoNotOptimize(
            directory.apply(
                fgep::venue::VenueId{1},
                stock_directory(stock_locate, "AAPL")
            )
        );

        const auto message = trading_action(
            stock_locate,
            "AAPL",
            fgep::itch::TradingState::trading
        );

        ++stock_locate;

        state.ResumeTiming();

        const auto error = directory.apply(fgep::venue::VenueId{1}, message);
        benchmark::DoNotOptimize(error);
    }
}

void BM_InstrumentDirectoryFindByStockLocate(benchmark::State& state) {
    fgep::instrument::InstrumentDirectory directory{};

    for (fgep::itch::StockLocate locate = 1; locate <= 1024; ++locate) {
        benchmark::DoNotOptimize(
            directory.apply(
                fgep::venue::VenueId{1},
                stock_directory(locate, "AAPL")
            )
        );
    }

    fgep::itch::StockLocate lookup = 1;

    for (auto _ : state) {
        const auto* metadata = directory.find(
            fgep::venue::VenueId{1},
            lookup
        );

        benchmark::DoNotOptimize(metadata);

        ++lookup;
        if (lookup > 1024) {
            lookup = 1;
        }
    }
}

} // namespace

BENCHMARK(BM_InstrumentDirectoryApplyStockDirectory);
BENCHMARK(BM_InstrumentDirectoryApplyTradingAction);
BENCHMARK(BM_InstrumentDirectoryFindByStockLocate);