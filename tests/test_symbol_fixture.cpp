#include "fgep/reference/symbol_fixture.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cassert>

namespace {

[[nodiscard]] fgep::ouch::Symbol symbol(const char* text) {
    const auto value = fgep::wire::make_fixed_ascii<8>(text);
    assert(value.ok());
    return value.value;
}

} // namespace

int main() {
    using namespace fgep::reference;

    {
        assert(trim_ascii_whitespace(" AAPL ") == "AAPL");
        assert(trim_ascii_whitespace("\tMSFT\r") == "MSFT");
        assert(trim_ascii_whitespace("") == "");
    }

    {
        assert(first_pipe_field("AAPL|Apple Inc.") == "AAPL");
        assert(first_pipe_field("MSFT") == "MSFT");
        assert(first_pipe_field("|empty") == "");
    }

    {
        assert(is_valid_fixture_symbol_text("AAPL"));
        assert(is_valid_fixture_symbol_text("BRK.B"));
        assert(is_valid_fixture_symbol_text("ABC123"));
        assert(is_valid_fixture_symbol_text("ABCDEFGH"));

        assert(!is_valid_fixture_symbol_text(""));
        assert(!is_valid_fixture_symbol_text("ABCDEFGHI"));
        assert(!is_valid_fixture_symbol_text("bad"));
        assert(!is_valid_fixture_symbol_text("AAPL "));
        assert(!is_valid_fixture_symbol_text("AAPL$"));
    }

    {
        const auto fixture = parse_symbol_fixture_text(
            "AAPL\nMSFT\nTSLA\n"
        );

        assert(!fixture.empty());
        assert(fixture.size() == 3);
        assert(fixture.symbols[0] == symbol("AAPL"));
        assert(fixture.symbols[1] == symbol("MSFT"));
        assert(fixture.symbols[2] == symbol("TSLA"));
    }

    {
        const auto fixture = parse_symbol_fixture_text(
            "# comment\n"
            "\n"
            " AAPL \n"
            "INVALID-LONG-SYMBOL\n"
            "bad\n"
            "MSFT\n"
        );

        assert(fixture.size() == 2);
        assert(fixture.symbols[0] == symbol("AAPL"));
        assert(fixture.symbols[1] == symbol("MSFT"));
    }

    {
        const auto fixture = parse_symbol_fixture_text(
            "Symbol|Security Name|ETF|Test Issue\n"
            "AAPL|Apple Inc.|N|N\n"
            "MSFT|Microsoft Corp.|N|N\n"
            "TSLA|Tesla Inc.|N|N\n"
        );

        assert(fixture.size() == 3);
        assert(fixture.symbols[0] == symbol("AAPL"));
        assert(fixture.symbols[1] == symbol("MSFT"));
        assert(fixture.symbols[2] == symbol("TSLA"));
    }

    {
        const auto fixture = parse_symbol_fixture_text(
            "AAPL\nMSFT\nTSLA\nNVDA\n",
            SymbolFixtureParseOptions{
                .max_symbols = 2,
                .allow_pipe_delimited_rows = true,
                .skip_header_rows = true
            }
        );

        assert(fixture.size() == 2);
        assert(fixture.symbols[0] == symbol("AAPL"));
        assert(fixture.symbols[1] == symbol("MSFT"));
    }

    {
    const auto fixture = parse_symbol_fixture_text(
        "SYMBOL\nAAPL\n",
        SymbolFixtureParseOptions{
            .max_symbols = 0,
            .allow_pipe_delimited_rows = true,
            .skip_header_rows = false
        }
    );

    assert(fixture.size() == 2);
    assert(fixture.symbols[0] == symbol("SYMBOL"));
    assert(fixture.symbols[1] == symbol("AAPL"));
    }

    return 0;
}