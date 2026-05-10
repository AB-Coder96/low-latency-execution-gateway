#include "fgep/book/multi_venue_book.hpp"

namespace fgep::book {

MultiVenueBook::MultiVenueBook(
    const instrument::InstrumentDirectory& directory
)
    : directory_{&directory} {
}

ErrorCode MultiVenueBook::add_venue(venue::VenueId venue_id) {
    if (!venue::is_valid_venue_id(venue_id) || directory_ == nullptr) {
        return ErrorCode::invalid_argument;
    }

    if (venues_.contains(venue_id)) {
        return ErrorCode::duplicate;
    }

    venues_.try_emplace(venue_id, venue_id, *directory_);

    return ErrorCode::ok;
}

ErrorCode MultiVenueBook::apply(
    venue::VenueId venue_id,
    const itch::Message& message
) {
    const auto existing = venues_.find(venue_id);

    if (existing == venues_.end()) {
        return ErrorCode::not_found;
    }

    return existing->second.apply(message);
}

std::optional<Quote> MultiVenueBook::best_bid(
    const instrument::InstrumentKey& key
) const {
    const auto venue = venues_.find(key.venue_id);

    if (venue == venues_.end()) {
        return std::nullopt;
    }

    return venue->second.best_bid(key);
}

std::optional<Quote> MultiVenueBook::best_ask(
    const instrument::InstrumentKey& key
) const {
    const auto venue = venues_.find(key.venue_id);

    if (venue == venues_.end()) {
        return std::nullopt;
    }

    return venue->second.best_ask(key);
}

std::vector<Quote> MultiVenueBook::bids(
    const instrument::CanonicalSymbol& symbol
) const {
    std::vector<Quote> quotes{};

    if (directory_ == nullptr) {
        return quotes;
    }

    for (const auto& key : directory_->instruments_for_symbol(symbol)) {
        const auto quote = best_bid(key);

        if (quote.has_value()) {
            quotes.push_back(*quote);
        }
    }

    return quotes;
}

std::vector<Quote> MultiVenueBook::asks(
    const instrument::CanonicalSymbol& symbol
) const {
    std::vector<Quote> quotes{};

    if (directory_ == nullptr) {
        return quotes;
    }

    for (const auto& key : directory_->instruments_for_symbol(symbol)) {
        const auto quote = best_ask(key);

        if (quote.has_value()) {
            quotes.push_back(*quote);
        }
    }

    return quotes;
}

bool MultiVenueBook::contains_venue(
    venue::VenueId venue_id
) const noexcept {
    return venues_.contains(venue_id);
}

std::size_t MultiVenueBook::venue_count() const noexcept {
    return venues_.size();
}

} // namespace fgep::book