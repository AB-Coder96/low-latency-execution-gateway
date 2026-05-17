#pragma once

#include "fgep/venue/venue.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

namespace fgep::reference {

struct VenueFixture {
    std::vector<venue::Venue> venues{};

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
};

struct VenueFixtureParseOptions {
    std::size_t max_venues{};
    bool skip_header_rows{true};
};

[[nodiscard]] VenueFixture parse_venue_fixture_text(
    std::string_view text,
    VenueFixtureParseOptions options = {}
);

[[nodiscard]] bool is_valid_venue_id_text(std::string_view text) noexcept;

[[nodiscard]] bool is_venue_header_row(std::string_view first_field) noexcept;

[[nodiscard]] std::string_view csv_field(
    std::string_view line,
    std::size_t field_index
) noexcept;

} // namespace fgep::reference