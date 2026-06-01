#include "fgep/book/symbol_order_book.hpp"
#include "fgep/instrument/instrument_key.hpp"
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

[[nodiscard]] fgep::book::SymbolOrderBook make_book() {
    return fgep::book::SymbolOrderBook{
        fgep::instrument::InstrumentKey{
            .venue_id = 1,
            .stock_locate = 1
        },
        stock_symbol("AAPL")
    };
}

[[nodiscard]] fgep::itch::Header header() {
    return fgep::itch::Header{
        .stock_locate = 1,
        .tracking_number = 1,
        .timestamp_ns = 100
    };
}

[[nodiscard]] fgep::itch::AddOrderNoMpidMessage add_order(
    fgep::itch::OrderReferenceNumber order_reference_number,
    fgep::itch::Side side,
    fgep::itch::Shares shares,
    fgep::itch::Price4 price
) {
    return fgep::itch::AddOrderNoMpidMessage{
        .header = header(),
        .order_reference_number = order_reference_number,
        .side = side,
        .shares = shares,
        .stock = stock_symbol("AAPL"),
        .price = price
    };
}

[[nodiscard]] fgep::itch::OrderCancelMessage cancel_order(
    fgep::itch::OrderReferenceNumber order_reference_number,
    fgep::itch::Shares cancelled_shares
) {
    return fgep::itch::OrderCancelMessage{
        .header = header(),
        .order_reference_number = order_reference_number,
        .cancelled_shares = cancelled_shares
    };
}

[[nodiscard]] fgep::itch::OrderDeleteMessage delete_order(
    fgep::itch::OrderReferenceNumber order_reference_number
) {
    return fgep::itch::OrderDeleteMessage{
        .header = header(),
        .order_reference_number = order_reference_number
    };
}

void BM_SymbolOrderBookApplyAddOrder(benchmark::State& state) {
    auto book = make_book();
    fgep::itch::OrderReferenceNumber order_ref = 1;

    for (auto _ : state) {
        const auto message = add_order(
            order_ref++,
            fgep::itch::Side::buy,
            100,
            1'902'500
        );

        const auto error = book.apply(message);

        benchmark::DoNotOptimize(error);

        if (book.order_count() >= 4096U) {
            state.PauseTiming();
            book = make_book();
            order_ref = 1;
            state.ResumeTiming();
        }
    }
}

void BM_SymbolOrderBookApplyCancelOrder(benchmark::State& state) {
    fgep::itch::OrderReferenceNumber order_ref = 1;

    for (auto _ : state) {
        state.PauseTiming();

        auto book = make_book();
        const auto ref = order_ref++;
        benchmark::DoNotOptimize(
            book.apply(
                add_order(ref, fgep::itch::Side::buy, 100, 1'902'500)
            )
        );

        state.ResumeTiming();

        const auto error = book.apply(cancel_order(ref, 50));
        benchmark::DoNotOptimize(error);
    }
}

void BM_SymbolOrderBookApplyDeleteOrder(benchmark::State& state) {
    fgep::itch::OrderReferenceNumber order_ref = 1;

    for (auto _ : state) {
        state.PauseTiming();

        auto book = make_book();
        const auto ref = order_ref++;
        benchmark::DoNotOptimize(
            book.apply(
                add_order(ref, fgep::itch::Side::sell, 100, 1'903'000)
            )
        );

        state.ResumeTiming();

        const auto error = book.apply(delete_order(ref));
        benchmark::DoNotOptimize(error);
    }
}

void BM_SymbolOrderBookBestBidAsk(benchmark::State& state) {
    auto book = make_book();

    benchmark::DoNotOptimize(
        book.apply(add_order(1, fgep::itch::Side::buy, 100, 1'902'500))
    );
    benchmark::DoNotOptimize(
        book.apply(add_order(2, fgep::itch::Side::sell, 100, 1'903'000))
    );

    for (auto _ : state) {
        const auto best_bid = book.best_bid();
        const auto best_ask = book.best_ask();

        benchmark::DoNotOptimize(best_bid);
        benchmark::DoNotOptimize(best_ask);
    }
}

} // namespace

BENCHMARK(BM_SymbolOrderBookApplyAddOrder);
BENCHMARK(BM_SymbolOrderBookApplyCancelOrder);
BENCHMARK(BM_SymbolOrderBookApplyDeleteOrder);
BENCHMARK(BM_SymbolOrderBookBestBidAsk);