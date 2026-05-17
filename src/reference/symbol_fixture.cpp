#include "fgep/reference/symbol_fixture.hpp"

#include "fgep/wire/fixed_ascii.hpp"

#include <cstddef>

namespace fgep::reference {
namespace {

[[nodiscard]] bool is_ascii_space(char value) noexcept {
    return value == ' '
        || value == '\t'
        || value == '\r'
        || value == '\n';
}

[[nodiscard]] bool is_uppercase_alpha(char value) noexcept {
    return value >= 'A' && value <= 'Z';
}

[[nodiscard]] bool is_digit(char value) noexcept {
    return value >= '0' && value <= '9';
}

[[nodiscard]] bool should_skip_header(std::string_view field) noexcept {
    return field == "Symbol"
        || field == "ACT Symbol"
        || field == "NASDAQ Symbol";
}

[[nodiscard]] bool should_skip_line(std::string_view line) noexcept {
    return line.empty() || line.front() == '#';
}

void append_symbol_if_valid(
    SymbolFixture& fixture,
    std::string_view field,
    const SymbolFixtureParseOptions& options
) {
    if (options.max_symbols != 0 && fixture.symbols.size() >= options.max_symbols) {
        return;
    }

    if (options.skip_header_rows && should_skip_header(field)) {
        return;
    }

    if (!is_valid_fixture_symbol_text(field)) {
        return;
    }

    const auto symbol = wire::make_fixed_ascii<8>(field);

    if (symbol.ok()) {
        fixture.symbols.push_back(symbol.value);
    }
}

} // namespace

bool SymbolFixture::empty() const noexcept {
    return symbols.empty();
}

std::size_t SymbolFixture::size() const noexcept {
    return symbols.size();
}

SymbolFixture parse_symbol_fixture_text(
    std::string_view text,
    SymbolFixtureParseOptions options
) {
    SymbolFixture fixture{};

    std::size_t offset = 0;

    while (offset <= text.size()) {
        const auto next_newline = text.find('\n', offset);
        const auto line_end = next_newline == std::string_view::npos
            ? text.size()
            : next_newline;

        auto line = trim_ascii_whitespace(
            text.substr(offset, line_end - offset)
        );

        if (!should_skip_line(line)) {
            if (options.allow_pipe_delimited_rows) {
                line = first_pipe_field(line);
            }

            line = trim_ascii_whitespace(line);
            append_symbol_if_valid(fixture, line, options);
        }

        if (next_newline == std::string_view::npos) {
            break;
        }

        offset = next_newline + 1U;
    }

    return fixture;
}

bool is_valid_fixture_symbol_text(std::string_view symbol) noexcept {
    if (symbol.empty() || symbol.size() > 8U) {
        return false;
    }

    for (const char value : symbol) {
        if (!is_uppercase_alpha(value) && !is_digit(value) && value != '.') {
            return false;
        }
    }

    return true;
}

std::string_view trim_ascii_whitespace(std::string_view text) noexcept {
    while (!text.empty() && is_ascii_space(text.front())) {
        text.remove_prefix(1);
    }

    while (!text.empty() && is_ascii_space(text.back())) {
        text.remove_suffix(1);
    }

    return text;
}

std::string_view first_pipe_field(std::string_view line) noexcept {
    const auto delimiter = line.find('|');

    if (delimiter == std::string_view::npos) {
        return line;
    }

    return line.substr(0, delimiter);
}

} // namespace fgep::reference