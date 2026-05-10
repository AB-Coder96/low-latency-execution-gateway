
#pragma once

#include "fgep/book/book_types.hpp"
#include "fgep/book/symbol_order_book.hpp"
#include "fgep/core/errors.hpp"
#include "fgep/instrument/instrument_directory.hpp"
#include "fgep/venue/venue.hpp"

#include <optional>
#include <unordered_map>

namespace fgep::book {

class VenueBook {
public:
    VenueBook(
        venue::VenueId venue_id,
        const instrument::InstrumentDirectory& directory
    );

    [[nodiscard]] venue::VenueId venue_id() const noexcept;

    [[nodiscard]] ErrorCode apply(const itch::Message& message);

    [[nodiscard]] std::optional<Quote> best_bid(
        const instrument::InstrumentKey& key
    ) const;

    [[nodiscard]] std::optional<Quote> best_ask(
        const instrument::InstrumentKey& key
    ) const;

    [[nodiscard]] bool contains_symbol(
        itch::StockLocate stock_locate
    ) const noexcept;

    [[nodiscard]] std::size_t symbol_count() const noexcept;

private:
    [[nodiscard]] const instrument::InstrumentMetadata* metadata_for(
        itch::StockLocate stock_locate
    ) const noexcept;

    [[nodiscard]] SymbolOrderBook* get_or_create_book(
        const instrument::InstrumentMetadata& metadata
    );

    [[nodiscard]] static bool affects_order_book(
        const itch::Message& message
    ) noexcept;

    [[nodiscard]] static itch::StockLocate stock_locate_from(
        const itch::Message& message
    ) noexcept;

    [[nodiscard]] Quote make_quote(
        const instrument::InstrumentMetadata& metadata,
        const Level& level
    ) const noexcept;

    venue::VenueId venue_id_{};
    const instrument::InstrumentDirectory* directory_{};

    std::unordered_map<itch::StockLocate, SymbolOrderBook> books_{};
};

} // namespace fgep::book