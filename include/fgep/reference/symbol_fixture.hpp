#pragma once

#include "fgep/ouch/ouch_wire_messages.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

namespace fgep::reference {

struct SymbolFixture {
    std::vector<ouch::Symbol> symbols{};

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
};

struct SymbolFixtureParseOptions {
    std::size_t max_symbols{};
    bool allow_pipe_delimited_rows{true};
    bool skip_header_rows{true};
};

[[nodiscard]] SymbolFixture parse_symbol_fixture_text(
    std::string_view text,
    SymbolFixtureParseOptions options = {}
);

[[nodiscard]] bool is_valid_fixture_symbol_text(
    std::string_view symbol
) noexcept;

[[nodiscard]] std::string_view trim_ascii_whitespace(
    std::string_view text
) noexcept;

[[nodiscard]] std::string_view first_pipe_field(
    std::string_view line
) noexcept;

} 