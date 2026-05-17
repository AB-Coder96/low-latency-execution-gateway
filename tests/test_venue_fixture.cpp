#include "fgep/reference/venue_fixture.hpp"

#include <cassert>

namespace {

[[nodiscard]] fgep::venue::Mic mic(const char* text) {
    const auto value = fgep::venue::make_mic(text);
    assert(value.ok());
    return value.value;
}

} // namespace

int main() {
    using namespace fgep::reference;

    {
        assert(csv_field("1,XNAS,Nasdaq", 0) == "1");
        assert(csv_field("1,XNAS,Nasdaq", 1) == "XNAS");
        assert(csv_field("1,XNAS,Nasdaq", 2) == "Nasdaq");
        assert(csv_field("1,XNAS,Nasdaq", 3).empty());
        assert(csv_field(" 2 , XNYS , NYSE ", 0) == "2");
        assert(csv_field(" 2 , XNYS , NYSE ", 1) == "XNYS");
    }

    {
        assert(is_valid_venue_id_text("1"));
        assert(is_valid_venue_id_text("65535"));
        assert(is_valid_venue_id_text(" 42 "));

        assert(!is_valid_venue_id_text(""));
        assert(!is_valid_venue_id_text("0x10"));
        assert(!is_valid_venue_id_text("A1"));
    }

    {
        assert(is_venue_header_row("venue_id"));
        assert(is_venue_header_row("VenueId"));
        assert(is_venue_header_row("id"));
        assert(is_venue_header_row("ID"));
        assert(!is_venue_header_row("1"));
    }

    {
        const auto fixture = parse_venue_fixture_text(
            "venue_id,mic,name\n"
            "1,XNAS,Nasdaq\n"
            "2,XNYS,NYSE\n"
        );

        assert(!fixture.empty());
        assert(fixture.size() == 2);

        assert(fixture.venues[0].id == 1);
        assert(fixture.venues[0].mic == mic("XNAS"));

        assert(fixture.venues[1].id == 2);
        assert(fixture.venues[1].mic == mic("XNYS"));
    }

    {
        const auto fixture = parse_venue_fixture_text(
            "# comment\n"
            "\n"
            "1,XNAS,Nasdaq\n"
            "0,XBAD,Invalid zero id\n"
            "999999,XBIG,Invalid large id\n"
            "3,bad,Invalid lowercase mic\n"
            "4,BAD,Invalid short mic\n"
            "5,ARCX,NYSE Arca\n"
        );

        assert(fixture.size() == 2);

        assert(fixture.venues[0].id == 1);
        assert(fixture.venues[0].mic == mic("XNAS"));

        assert(fixture.venues[1].id == 5);
        assert(fixture.venues[1].mic == mic("ARCX"));
    }

    {
        const auto fixture = parse_venue_fixture_text(
            "1,XNAS,Nasdaq\n"
            "2,XNYS,NYSE\n"
            "3,ARCX,NYSE Arca\n",
            VenueFixtureParseOptions{
                .max_venues = 2,
                .skip_header_rows = true
            }
        );

        assert(fixture.size() == 2);
        assert(fixture.venues[0].id == 1);
        assert(fixture.venues[1].id == 2);
    }

    {
        const auto fixture = parse_venue_fixture_text(
            "id,mic,name\n"
            "1,XNAS,Nasdaq\n",
            VenueFixtureParseOptions{
                .max_venues = 0,
                .skip_header_rows = false
            }
        );

        assert(fixture.size() == 1);
        assert(fixture.venues[0].id == 1);
        assert(fixture.venues[0].mic == mic("XNAS"));
    }

    return 0;
}