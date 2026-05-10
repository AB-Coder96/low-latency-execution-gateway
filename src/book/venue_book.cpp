#include "fgep/book/venue_book.hpp"

#include <type_traits>
#include <variant>

namespace fgep::book {

VenueBook::VenueBook(
    venue::VenueId venue_id,
    const instrument::InstrumentDirectory& directory
)
    : venue_id_{venue_id},
      directory_{&directory} {
}

venue::VenueId VenueBook::venue_id() const noexcept {
    return venue_id_;
}

ErrorCode VenueBook::apply(const itch::Message& message) {
    if (!venue::is_valid_venue_id(venue_id_) || directory_ == nullptr) {
        return ErrorCode::invalid_state;
    }

    if (!affects_order_book(message)) {
        return ErrorCode::ok;
    }

    const auto stock_locate = stock_locate_from(message);
    const auto* metadata = metadata_for(stock_locate);

    if (metadata == nullptr) {
        return ErrorCode::not_found;
    }

    auto* book = get_or_create_book(*metadata);

    if (book == nullptr) {
        return ErrorCode::internal_error;
    }

    return book->apply(message);
}

std::optional<Quote> VenueBook::best_bid(
    const instrument::InstrumentKey& key
) const {
    if (key.venue_id != venue_id_) {
        return std::nullopt;
    }

    const auto book = books_.find(key.stock_locate);

    if (book == books_.end()) {
        return std::nullopt;
    }

    const auto level = book->second.best_bid();

    if (!level.has_value()) {
        return std::nullopt;
    }

    const auto* metadata = metadata_for(key.stock_locate);

    if (metadata == nullptr) {
        return std::nullopt;
    }

    return make_quote(*metadata, *level);
}

std::optional<Quote> VenueBook::best_ask(
    const instrument::InstrumentKey& key
) const {
    if (key.venue_id != venue_id_) {
        return std::nullopt;
    }

    const auto book = books_.find(key.stock_locate);

    if (book == books_.end()) {
        return std::nullopt;
    }

    const auto level = book->second.best_ask();

    if (!level.has_value()) {
        return std::nullopt;
    }

    const auto* metadata = metadata_for(key.stock_locate);

    if (metadata == nullptr) {
        return std::nullopt;
    }

    return make_quote(*metadata, *level);
}

bool VenueBook::contains_symbol(
    itch::StockLocate stock_locate
) const noexcept {
    return books_.contains(stock_locate);
}

std::size_t VenueBook::symbol_count() const noexcept {
    return books_.size();
}

const instrument::InstrumentMetadata* VenueBook::metadata_for(
    itch::StockLocate stock_locate
) const noexcept {
    if (directory_ == nullptr) {
        return nullptr;
    }

    return directory_->find(venue_id_, stock_locate);
}

SymbolOrderBook* VenueBook::get_or_create_book(
    const instrument::InstrumentMetadata& metadata
) {
    const auto existing = books_.find(metadata.key.stock_locate);

    if (existing != books_.end()) {
        return &existing->second;
    }

    auto [inserted, inserted_new] = books_.try_emplace(
        metadata.key.stock_locate,
        metadata.key,
        metadata.stock
    );

    (void)inserted_new;

    return &inserted->second;
}

bool VenueBook::affects_order_book(const itch::Message& message) noexcept {
    return std::visit(
        [](const auto& concrete_message) noexcept -> bool {
            using MessageT = std::decay_t<decltype(concrete_message)>;

            return std::is_same_v<MessageT, itch::AddOrderNoMpidMessage>
                || std::is_same_v<MessageT, itch::AddOrderWithMpidMessage>
                || std::is_same_v<MessageT, itch::OrderExecutedMessage>
                || std::is_same_v<
                    MessageT,
                    itch::OrderExecutedWithPriceMessage
                >
                || std::is_same_v<MessageT, itch::OrderCancelMessage>
                || std::is_same_v<MessageT, itch::OrderDeleteMessage>
                || std::is_same_v<MessageT, itch::OrderReplaceMessage>;
        },
        message
    );
}

itch::StockLocate VenueBook::stock_locate_from(
    const itch::Message& message
) noexcept {
    return std::visit(
        [](const auto& concrete_message) noexcept -> itch::StockLocate {
            return concrete_message.header.stock_locate;
        },
        message
    );
}

Quote VenueBook::make_quote(
    const instrument::InstrumentMetadata& metadata,
    const Level& level
) const noexcept {
    return Quote{
        metadata.key,
        instrument::CanonicalSymbol{metadata.stock},
        level.price,
        level.quantity,
        level.order_count
    };
}

} // namespace fgep::book