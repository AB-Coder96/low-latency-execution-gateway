#pragma once

#include "fgep/book/venue_book.hpp"
#include "fgep/core/errors.hpp"
#include "fgep/instrument/instrument_directory.hpp"
#include "fgep/venue/venue.hpp"

#include <unordered_map>
#include <vector>

namespace fgep::book {

class MultiVenueBook {
public:
    explicit MultiVenueBook(
        const instrument::InstrumentDirectory& directory
    );

    [[nodiscard]] ErrorCode add_venue(venue::VenueId venue_id);

    [[nodiscard]] ErrorCode apply(
        venue::VenueId venue_id,
        const itch::Message& message
    );

    [[nodiscard]] std::optional<Quote> best_bid(
        const instrument::InstrumentKey& key
    ) const;

    [[nodiscard]] std::optional<Quote> best_ask(
        const instrument::InstrumentKey& key
    ) const;

    [[nodiscard]] std::vector<Quote> bids(
        const instrument::CanonicalSymbol& symbol
    ) const;

    [[nodiscard]] std::vector<Quote> asks(
        const instrument::CanonicalSymbol& symbol
    ) const;

    [[nodiscard]] bool contains_venue(
        venue::VenueId venue_id
    ) const noexcept;

    [[nodiscard]] std::size_t venue_count() const noexcept;

private:
    const instrument::InstrumentDirectory* directory_{};
    std::unordered_map<venue::VenueId, VenueBook> venues_{};
};

} // namespace fgep::book