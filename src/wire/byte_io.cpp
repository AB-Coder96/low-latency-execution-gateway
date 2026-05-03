#include "fgep/wire/byte_io.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

int main() {
    using namespace fgep;
    using namespace fgep::wire;

    // -------------------------------------------------------------------------
    // Range checks
    // -------------------------------------------------------------------------

    const std::array<std::byte, 4> small_const_buffer{
        std::byte{0x00},
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03}
    };

    assert(has_range(std::span<const std::byte>{small_const_buffer}, 0, 4));
    assert(has_range(std::span<const std::byte>{small_const_buffer}, 2, 2));
    assert(!has_range(std::span<const std::byte>{small_const_buffer}, 3, 2));
    assert(!has_range(std::span<const std::byte>{small_const_buffer}, 5, 1));

    // -------------------------------------------------------------------------
    // read_u8
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, 1> bytes{
            std::byte{0xAB}
        };

        const auto result = read_u8(bytes, 0);

        assert(result.ok());
        assert(result.value == 0xABU);

        const auto bad_result = read_u8(bytes, 1);

        assert(bad_result.failed());
        assert(bad_result.error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // read_u16_be
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, 2> bytes{
            std::byte{0x12},
            std::byte{0x34}
        };

        const auto result = read_u16_be(bytes, 0);

        assert(result.ok());
        assert(result.value == 0x1234U);

        const auto bad_result = read_u16_be(bytes, 1);

        assert(bad_result.failed());
        assert(bad_result.error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // read_u32_be
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, 4> bytes{
            std::byte{0x12},
            std::byte{0x34},
            std::byte{0x56},
            std::byte{0x78}
        };

        const auto result = read_u32_be(bytes, 0);

        assert(result.ok());
        assert(result.value == 0x12345678UL);

        const auto bad_result = read_u32_be(bytes, 1);

        assert(bad_result.failed());
        assert(bad_result.error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // read_u48_be
    // -------------------------------------------------------------------------
    //
    // ITCH timestamps are 6-byte unsigned integers, so this is required for
    // byte-to-byte ITCH support.

    {
        const std::array<std::byte, 6> bytes{
            std::byte{0x01},
            std::byte{0x23},
            std::byte{0x45},
            std::byte{0x67},
            std::byte{0x89},
            std::byte{0xAB}
        };

        const auto result = read_u48_be(bytes, 0);

        assert(result.ok());
        assert(result.value == 0x0123456789ABULL);

        const auto bad_result = read_u48_be(bytes, 1);

        assert(bad_result.failed());
        assert(bad_result.error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // read_u64_be
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, 8> bytes{
            std::byte{0x01},
            std::byte{0x23},
            std::byte{0x45},
            std::byte{0x67},
            std::byte{0x89},
            std::byte{0xAB},
            std::byte{0xCD},
            std::byte{0xEF}
        };

        const auto result = read_u64_be(bytes, 0);

        assert(result.ok());
        assert(result.value == 0x0123456789ABCDEFULL);

        const auto bad_result = read_u64_be(bytes, 1);

        assert(bad_result.failed());
        assert(bad_result.error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // write_u8
    // -------------------------------------------------------------------------

    {
        std::array<std::byte, 1> bytes{};

        const auto error = write_u8(bytes, 0, 0xCDU);

        assert(error == ErrorCode::ok);
        assert(bytes[0] == std::byte{0xCD});

        const auto bad_error = write_u8(bytes, 1, 0xEFU);

        assert(bad_error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // write_u16_be
    // -------------------------------------------------------------------------

    {
        std::array<std::byte, 2> bytes{};

        const auto error = write_u16_be(bytes, 0, 0x1234U);

        assert(error == ErrorCode::ok);
        assert(bytes[0] == std::byte{0x12});
        assert(bytes[1] == std::byte{0x34});

        const auto round_trip = read_u16_be(bytes, 0);

        assert(round_trip.ok());
        assert(round_trip.value == 0x1234U);

        const auto bad_error = write_u16_be(bytes, 1, 0x1234U);

        assert(bad_error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // write_u32_be
    // -------------------------------------------------------------------------

    {
        std::array<std::byte, 4> bytes{};

        const auto error = write_u32_be(bytes, 0, 0x12345678UL);

        assert(error == ErrorCode::ok);
        assert(bytes[0] == std::byte{0x12});
        assert(bytes[1] == std::byte{0x34});
        assert(bytes[2] == std::byte{0x56});
        assert(bytes[3] == std::byte{0x78});

        const auto round_trip = read_u32_be(bytes, 0);

        assert(round_trip.ok());
        assert(round_trip.value == 0x12345678UL);

        const auto bad_error = write_u32_be(bytes, 1, 0x12345678UL);

        assert(bad_error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // write_u48_be
    // -------------------------------------------------------------------------

    {
        std::array<std::byte, 6> bytes{};

        const auto error = write_u48_be(bytes, 0, 0x0123456789ABULL);

        assert(error == ErrorCode::ok);
        assert(bytes[0] == std::byte{0x01});
        assert(bytes[1] == std::byte{0x23});
        assert(bytes[2] == std::byte{0x45});
        assert(bytes[3] == std::byte{0x67});
        assert(bytes[4] == std::byte{0x89});
        assert(bytes[5] == std::byte{0xAB});

        const auto round_trip = read_u48_be(bytes, 0);

        assert(round_trip.ok());
        assert(round_trip.value == 0x0123456789ABULL);

        const auto bad_range_error = write_u48_be(bytes, 1, 0x0123456789ABULL);

        assert(bad_range_error == ErrorCode::parse_error);

        const auto overflow_error = write_u48_be(bytes, 0, 0x0001000000000000ULL);

        assert(overflow_error == ErrorCode::invalid_argument);
    }

    // -------------------------------------------------------------------------
    // write_u64_be
    // -------------------------------------------------------------------------

    {
        std::array<std::byte, 8> bytes{};

        const auto error = write_u64_be(bytes, 0, 0x0123456789ABCDEFULL);

        assert(error == ErrorCode::ok);
        assert(bytes[0] == std::byte{0x01});
        assert(bytes[1] == std::byte{0x23});
        assert(bytes[2] == std::byte{0x45});
        assert(bytes[3] == std::byte{0x67});
        assert(bytes[4] == std::byte{0x89});
        assert(bytes[5] == std::byte{0xAB});
        assert(bytes[6] == std::byte{0xCD});
        assert(bytes[7] == std::byte{0xEF});

        const auto round_trip = read_u64_be(bytes, 0);

        assert(round_trip.ok());
        assert(round_trip.value == 0x0123456789ABCDEFULL);

        const auto bad_error = write_u64_be(bytes, 1, 0x0123456789ABCDEFULL);

        assert(bad_error == ErrorCode::parse_error);
    }

    return 0;
}