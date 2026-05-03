#include "fgep/wire/fixed_ascii.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <string_view>

int main() {
    using namespace fgep;
    using namespace fgep::wire;

    // -------------------------------------------------------------------------
    // Printable ASCII validation
    // -------------------------------------------------------------------------

    assert(is_printable_ascii_or_space('A'));
    assert(is_printable_ascii_or_space('z'));
    assert(is_printable_ascii_or_space('0'));
    assert(is_printable_ascii_or_space(' '));
    assert(is_printable_ascii_or_space('~'));

    assert(!is_printable_ascii_or_space('\n'));
    assert(!is_printable_ascii_or_space('\t'));

    // -------------------------------------------------------------------------
    // make_fixed_ascii pads on the right with spaces
    // -------------------------------------------------------------------------

    {
        const auto result = make_fixed_ascii<8>("AAPL");

        assert(result.ok());

        const auto value = result.value;

        assert(value[0] == 'A');
        assert(value[1] == 'A');
        assert(value[2] == 'P');
        assert(value[3] == 'L');
        assert(value[4] == ' ');
        assert(value[5] == ' ');
        assert(value[6] == ' ');
        assert(value[7] == ' ');

        assert(trimmed_length(value) == 4);
        assert(fixed_ascii_equals(value, "AAPL"));
        assert(!fixed_ascii_equals(value, "AAPL "));
        assert(!fixed_ascii_equals(value, "MSFT"));
    }

    // -------------------------------------------------------------------------
    // make_fixed_ascii accepts exact-width strings
    // -------------------------------------------------------------------------

    {
        const auto result = make_fixed_ascii<4>("NSDQ");

        assert(result.ok());

        const auto value = result.value;

        assert(value[0] == 'N');
        assert(value[1] == 'S');
        assert(value[2] == 'D');
        assert(value[3] == 'Q');

        assert(trimmed_length(value) == 4);
        assert(fixed_ascii_equals(value, "NSDQ"));
    }

    // -------------------------------------------------------------------------
    // make_fixed_ascii rejects strings that are too long
    // -------------------------------------------------------------------------

    {
        const auto result = make_fixed_ascii<4>("TOOLONG");

        assert(result.failed());
        assert(result.error == ErrorCode::invalid_argument);
    }

    // -------------------------------------------------------------------------
    // make_fixed_ascii rejects non-printable ASCII
    // -------------------------------------------------------------------------

    {
        const auto result = make_fixed_ascii<8>("BAD\n");

        assert(result.failed());
        assert(result.error == ErrorCode::invalid_argument);
    }

    // -------------------------------------------------------------------------
    // is_valid_fixed_ascii
    // -------------------------------------------------------------------------

    {
        const FixedAscii<4> valid_value{'N', 'S', 'D', 'Q'};
        const FixedAscii<4> invalid_value{'N', 'S', '\n', 'Q'};

        assert(is_valid_fixed_ascii(valid_value));
        assert(!is_valid_fixed_ascii(invalid_value));
    }

    // -------------------------------------------------------------------------
    // read_fixed_ascii reads exact bytes from a buffer
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, 10> bytes{
            std::byte{'x'},
            std::byte{'x'},
            std::byte{'A'},
            std::byte{'A'},
            std::byte{'P'},
            std::byte{'L'},
            std::byte{' '},
            std::byte{' '},
            std::byte{' '},
            std::byte{' '}
        };

        const auto result = read_fixed_ascii<8>(bytes, 2);

        assert(result.ok());
        assert(fixed_ascii_equals(result.value, "AAPL"));
        assert(trimmed_length(result.value) == 4);
    }

    // -------------------------------------------------------------------------
    // read_fixed_ascii rejects out-of-range reads
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, 4> bytes{
            std::byte{'A'},
            std::byte{'A'},
            std::byte{'P'},
            std::byte{'L'}
        };

        const auto result = read_fixed_ascii<8>(bytes, 0);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // read_fixed_ascii rejects non-printable bytes
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, 4> bytes{
            std::byte{'N'},
            std::byte{'S'},
            std::byte{0x0A},
            std::byte{'Q'}
        };

        const auto result = read_fixed_ascii<4>(bytes, 0);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // write_fixed_ascii writes exact bytes to a buffer
    // -------------------------------------------------------------------------

    {
        std::array<std::byte, 10> bytes{};
        const auto symbol = make_fixed_ascii<8>("MSFT");

        assert(symbol.ok());

        const auto error = write_fixed_ascii<8>(bytes, 1, symbol.value);

        assert(error == ErrorCode::ok);

        assert(bytes[0] == std::byte{0x00});
        assert(bytes[1] == std::byte{'M'});
        assert(bytes[2] == std::byte{'S'});
        assert(bytes[3] == std::byte{'F'});
        assert(bytes[4] == std::byte{'T'});
        assert(bytes[5] == std::byte{' '});
        assert(bytes[6] == std::byte{' '});
        assert(bytes[7] == std::byte{' '});
        assert(bytes[8] == std::byte{' '});
        assert(bytes[9] == std::byte{0x00});

        const auto round_trip = read_fixed_ascii<8>(bytes, 1);

        assert(round_trip.ok());
        assert(fixed_ascii_equals(round_trip.value, "MSFT"));
    }

    // -------------------------------------------------------------------------
    // write_fixed_ascii rejects out-of-range writes
    // -------------------------------------------------------------------------

    {
        std::array<std::byte, 4> bytes{};
        const FixedAscii<8> symbol{'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '};

        const auto error = write_fixed_ascii<8>(bytes, 0, symbol);

        assert(error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // write_fixed_ascii rejects invalid ASCII
    // -------------------------------------------------------------------------

    {
        std::array<std::byte, 4> bytes{};
        const FixedAscii<4> invalid_value{'N', 'S', '\n', 'Q'};

        const auto error = write_fixed_ascii<4>(bytes, 0, invalid_value);

        assert(error == ErrorCode::invalid_argument);
    }

    // -------------------------------------------------------------------------
    // fully space-padded field trims to zero length
    // -------------------------------------------------------------------------

    {
        const FixedAscii<8> blank_symbol{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

        assert(trimmed_length(blank_symbol) == 0);
        assert(fixed_ascii_equals(blank_symbol, ""));
    }

    return 0;
}