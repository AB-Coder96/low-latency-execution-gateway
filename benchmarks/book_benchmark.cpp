#include "hotpath_benchmark.hpp"

#include "fgep/book/symbol_order_book.hpp"
#include "fgep/instrument/instrument_key.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cstdint>

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

[[nodiscard]] fgep::itch::Header header(std::uint64_t sequence) {
    return fgep::itch::Header{
        .stock_locate = 1,
        .tracking_number = static_cast<fgep::itch::TrackingNumber>(
            sequence & 0xFFFFU
        ),
        .timestamp_ns = 1'000U + sequence
    };
}

[[nodiscard]] fgep::itch::AddOrderNoMpidMessage add_order(
    fgep::itch::OrderReferenceNumber order_reference_number,
    fgep::itch::Side side,
    fgep::itch::Shares shares,
    fgep::itch::Price4 price
) {
    return fgep::itch::AddOrderNoMpidMessage{
        .header = header(order_reference_number),
        .order_reference_number = order_reference_number,
        .side = side,
        .shares = shares,
        .stock = stock_symbol("AAPL"),
        .price = price
    };
}

} // namespace

int main() {
    constexpr std::uint64_t iterations = 5'000'000U;

    auto book = make_book();
    fgep::itch::OrderReferenceNumber next_ref = 1U;

    const auto result = fgep::benchmarks::run_hot_loop(
        "hotpath_book_apply_add_order",
        iterations,
        [&](std::uint64_t index) {
            const auto message = add_order(
                next_ref++,
                (index & 1U) == 0U ? fgep::itch::Side::buy : fgep::itch::Side::sell,
                100,
                static_cast<fgep::itch::Price4>(1'900'000U + (index % 128U))
            );

            const auto error = book.apply(message);

            fgep::benchmarks::do_not_optimize(error);
            fgep::benchmarks::clobber_memory();

            if (book.order_count() >= 4096U) {
                book = make_book();
                next_ref = 1U;
            }
        }
    );

    fgep::benchmarks::print_result(result);
    return 0;
}