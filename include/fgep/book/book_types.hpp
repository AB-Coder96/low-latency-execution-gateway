#pragma once

#include "fgep/core/types.hpp"
#include "fgep/instrument/instrument_key.hpp"

#include <cstddef>
#include <optional>

namespace fgep::book {

struct Level {
    Price price{};
    Quantity quantity{};
    std::size_t order_count{};
};

struct Quote {
    instrument::InstrumentKey key{};
    instrument::CanonicalSymbol symbol{};
    Price price{};
    Quantity quantity{};
    std::size_t order_count{};
};

struct Bbo {
    instrument::CanonicalSymbol symbol{};
    std::optional<Quote> bid{};
    std::optional<Quote> ask{};
};

[[nodiscard]] constexpr bool is_empty(const Level& level) noexcept {
    return level.quantity == 0 || level.order_count == 0;
}

} // namespace fgep::book