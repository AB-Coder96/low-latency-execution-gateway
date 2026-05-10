#pragma once

#include "fgep/book/book_types.hpp"
#include "fgep/book/multi_venue_book.hpp"
#include "fgep/instrument/instrument_key.hpp"

namespace fgep::bbo {

class NormalizedBboView {
public:
    explicit NormalizedBboView(const book::MultiVenueBook& books);

    [[nodiscard]] book::Bbo get(
        const instrument::CanonicalSymbol& symbol
    ) const;

private:
    [[nodiscard]] static bool is_better_bid(
        const book::Quote& candidate,
        const book::Quote& current
    ) noexcept;

    [[nodiscard]] static bool is_better_ask(
        const book::Quote& candidate,
        const book::Quote& current
    ) noexcept;

    const book::MultiVenueBook* books_{};
};

} // namespace fgep::bbo