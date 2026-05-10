#include "fgep/bbo/normalized_bbo.hpp"

namespace fgep::bbo {

NormalizedBboView::NormalizedBboView(const book::MultiVenueBook& books)
    : books_{&books} {
}

book::Bbo NormalizedBboView::get(
    const instrument::CanonicalSymbol& symbol
) const {
    book::Bbo bbo{};
    bbo.symbol = symbol;

    if (books_ == nullptr) {
        return bbo;
    }

    for (const auto& quote : books_->bids(symbol)) {
        if (!bbo.bid.has_value() || is_better_bid(quote, *bbo.bid)) {
            bbo.bid = quote;
        }
    }

    for (const auto& quote : books_->asks(symbol)) {
        if (!bbo.ask.has_value() || is_better_ask(quote, *bbo.ask)) {
            bbo.ask = quote;
        }
    }

    return bbo;
}

bool NormalizedBboView::is_better_bid(
    const book::Quote& candidate,
    const book::Quote& current
) noexcept {
    if (candidate.price != current.price) {
        return candidate.price > current.price;
    }

    if (candidate.quantity != current.quantity) {
        return candidate.quantity > current.quantity;
    }

    return candidate.key.venue_id < current.key.venue_id;
}

bool NormalizedBboView::is_better_ask(
    const book::Quote& candidate,
    const book::Quote& current
) noexcept {
    if (candidate.price != current.price) {
        return candidate.price < current.price;
    }

    if (candidate.quantity != current.quantity) {
        return candidate.quantity > current.quantity;
    }

    return candidate.key.venue_id < current.key.venue_id;
}

} // namespace fgep::bbo