#include "fgep/reference/venue_fixture.hpp"

#include "fgep/reference/symbol_fixture.hpp"

#include <cstdint>
#include <limits>
#include <optional>

namespace fgep::reference {
namespace {

[[nodiscard]] bool is_digit(char value) noexcept {
    return value >= '0' && value <= '9';
}

[[nodiscard]] bool should_skip_line(std::string_view line) noexcept {
    return line.empty() || line.front() == '#';
}

[[nodiscard]] std::optional<venue::VenueId> parse_venue_id(
    std::string_view text
) noexcept {
    text = trim_ascii_whitespace(text);

    if (!is_valid_venue_id_text(text)) {
        return std::nullopt;
    }

    std::uint32_t value = 0;

    for (const char character : text) {
        const auto digit = static_cast<std::uint32_t>(character - '0');

        if (
            value
            > (
                static_cast<std::uint32_t>(
                    std::numeric_limits<venue::VenueId>::max()
                ) - digit
            ) / 10U
        ) {
            return std::nullopt;
        }

        value = (value * 10U) + digit;
    }

    if (value == venue::invalid_venue_id) {
        return std::nullopt;
    }

    return static_cast<venue::VenueId>(value);
}

void append_venue_if_valid(
    VenueFixture& fixture,
    std::string_view id_field,
    std::string_view mic_field,
    const VenueFixtureParseOptions& options
) {
    if (options.max_venues != 0 && fixture.venues.size() >= options.max_venues) {
        return;
    }

    id_field = trim_ascii_whitespace(id_field);
    mic_field = trim_ascii_whitespace(mic_field);

    if (options.skip_header_rows && is_venue_header_row(id_field)) {
        return;
    }

    const auto venue_id = parse_venue_id(id_field);

    if (!venue_id.has_value()) {
        return;
    }

    const auto mic = venue::make_mic(mic_field);

    if (mic.failed()) {
        return;
    }

    fixture.venues.push_back(venue::Venue{
        .id = venue_id.value(),
        .mic = mic.value
    });
}

} // namespace

bool VenueFixture::empty() const noexcept {
    return venues.empty();
}

std::size_t VenueFixture::size() const noexcept {
    return venues.size();
}

VenueFixture parse_venue_fixture_text(
    std::string_view text,
    VenueFixtureParseOptions options
) {
    VenueFixture fixture{};

    std::size_t offset = 0;

    while (offset <= text.size()) {
        const auto next_newline = text.find('\n', offset);
        const auto line_end = next_newline == std::string_view::npos
            ? text.size()
            : next_newline;

        const auto line = trim_ascii_whitespace(
            text.substr(offset, line_end - offset)
        );

        if (!should_skip_line(line)) {
            append_venue_if_valid(
                fixture,
                csv_field(line, 0),
                csv_field(line, 1),
                options
            );
        }

        if (next_newline == std::string_view::npos) {
            break;
        }

        offset = next_newline + 1U;
    }

    return fixture;
}

bool is_valid_venue_id_text(std::string_view text) noexcept {
    text = trim_ascii_whitespace(text);

    if (text.empty()) {
        return false;
    }

    for (const char character : text) {
        if (!is_digit(character)) {
            return false;
        }
    }

    return true;
}

bool is_venue_header_row(std::string_view first_field) noexcept {
    first_field = trim_ascii_whitespace(first_field);

    return first_field == "venue_id"
        || first_field == "VenueId"
        || first_field == "id"
        || first_field == "ID";
}

std::string_view csv_field(
    std::string_view line,
    std::size_t field_index
) noexcept {
    std::size_t start = 0;

    for (std::size_t index = 0; index < field_index; ++index) {
        const auto delimiter = line.find(',', start);

        if (delimiter == std::string_view::npos) {
            return {};
        }

        start = delimiter + 1U;
    }

    const auto end = line.find(',', start);

    if (end == std::string_view::npos) {
        return trim_ascii_whitespace(line.substr(start));
    }

    return trim_ascii_whitespace(line.substr(start, end - start));
}

} // namespace fgep::reference