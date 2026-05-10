#pragma once

#include "fgep/venue/mic.hpp"

#include <cstdint>

namespace fgep::venue {

using VenueId = std::uint16_t;

inline constexpr VenueId invalid_venue_id = 0;

struct Venue {
    VenueId id{};
    Mic mic{};
};

[[nodiscard]] constexpr bool is_valid_venue_id(VenueId venue_id) noexcept {
    return venue_id != invalid_venue_id;
}

[[nodiscard]] constexpr bool is_valid_venue(const Venue& venue) noexcept {
    return is_valid_venue_id(venue.id) && is_valid_mic(venue.mic);
}

} // namespace fgep::venue