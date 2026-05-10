#pragma once

#include "fgep/itch/itch_types.hpp"
#include "fgep/venue/venue.hpp"

#include <cstddef>

namespace fgep::instrument {

struct InstrumentKey {
    venue::VenueId venue_id{};
    itch::StockLocate stock_locate{};

    [[nodiscard]] friend bool operator==(
        const InstrumentKey& lhs,
        const InstrumentKey& rhs
    ) noexcept {
        return lhs.venue_id == rhs.venue_id
            && lhs.stock_locate == rhs.stock_locate;
    }
};

struct CanonicalSymbol {
    itch::StockSymbol stock{};

    [[nodiscard]] friend bool operator==(
        const CanonicalSymbol& lhs,
        const CanonicalSymbol& rhs
    ) noexcept {
        return lhs.stock == rhs.stock;
    }
};

struct InstrumentKeyHash {
    [[nodiscard]] std::size_t operator()(
        const InstrumentKey& key
    ) const noexcept {
        return (static_cast<std::size_t>(key.venue_id) << 16U)
            ^ static_cast<std::size_t>(key.stock_locate);
    }
};

struct CanonicalSymbolHash {
    [[nodiscard]] std::size_t operator()(
        const CanonicalSymbol& symbol
    ) const noexcept {
        std::size_t hash = 0;

        for (const char character : symbol.stock) {
            hash = (hash * 131U)
                ^ static_cast<std::size_t>(
                    static_cast<unsigned char>(character)
                );
        }

        return hash;
    }
};

[[nodiscard]] constexpr bool is_valid_instrument_key(
    const InstrumentKey& key
) noexcept {
    return venue::is_valid_venue_id(key.venue_id)
        && itch::is_valid_stock_locate(key.stock_locate);
}

} // namespace fgep::instrument