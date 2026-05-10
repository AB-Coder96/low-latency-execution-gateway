#include "fgep/itch/itch_decode.hpp"
#include "fgep/itch/itch_encode.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>

namespace {

[[nodiscard]] constexpr std::byte ub(unsigned int value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

[[nodiscard]] constexpr std::byte cb(char value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

template <std::size_t N>
void assert_round_trip_message(const std::array<std::byte, N>& input) {
    const auto decoded = fgep::itch::decode_message(
        std::span<const std::byte>{input}
    );

    assert(decoded.ok());

    std::array<std::byte, N> output{};

    const auto encode_error = fgep::itch::encode_message(
        std::span<std::byte>{output},
        decoded.value
    );

    assert(encode_error == fgep::ErrorCode::ok);
    assert(output == input);
}

} // namespace

int main() {
    using namespace fgep;
    using namespace fgep::itch;

    // -------------------------------------------------------------------------
    // S - System Event Message
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, length_system_event> bytes{
            cb('S'),
            ub(0x00), ub(0x00),                         // stock locate = 0
            ub(0x00), ub(0x01),                         // tracking number = 1
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x01),
            cb('O')                                     // start of messages
        };

        const auto result = decode_system_event_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.header.stock_locate == 0);
        assert(message.header.tracking_number == 1);
        assert(message.header.timestamp_ns == 1);
        assert(message.event_code == SystemEventCode::start_of_messages);

        std::array<std::byte, length_system_event> encoded{};

        assert(
            encode_system_event_message(encoded, message)
            == ErrorCode::ok
        );
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    // -------------------------------------------------------------------------
    // A - Add Order, No MPID Attribution
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, length_add_order_no_mpid> bytes{
            cb('A'),
            ub(0x00), ub(0x01),                         // stock locate = 1
            ub(0x00), ub(0x02),                         // tracking number = 2
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x03), ub(0xE8),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x64),     // order ref = 100
            cb('B'),                                    // buy
            ub(0x00), ub(0x00), ub(0x00), ub(0xFA),     // shares = 250
            cb('A'), cb('A'), cb('P'), cb('L'), cb(' '), cb(' '), cb(' '), cb(' '),
            ub(0x00), ub(0x1D), ub(0x07), ub(0xA4)      // price = 1902500
        };

        const auto result = decode_add_order_no_mpid_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.header.stock_locate == 1);
        assert(message.header.tracking_number == 2);
        assert(message.header.timestamp_ns == 1000);
        assert(message.order_reference_number == 100);
        assert(message.side == fgep::itch::Side::buy);
        assert(message.shares == 250);
        assert(wire::fixed_ascii_equals(message.stock, "AAPL"));
        assert(message.price == 1902500);

        std::array<std::byte, length_add_order_no_mpid> encoded{};

        assert(
            encode_add_order_no_mpid_message(encoded, message)
            == ErrorCode::ok
        );
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    // -------------------------------------------------------------------------
    // F - Add Order with MPID Attribution
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, length_add_order_with_mpid> bytes{
            cb('F'),
            ub(0x00), ub(0x02),                         // stock locate = 2
            ub(0x00), ub(0x03),                         // tracking number = 3
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x07), ub(0xD0),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x65),     // order ref = 101
            cb('S'),                                    // sell
            ub(0x00), ub(0x00), ub(0x00), ub(0x64),     // shares = 100
            cb('M'), cb('S'), cb('F'), cb('T'), cb(' '), cb(' '), cb(' '), cb(' '),
            ub(0x00), ub(0x40), ub(0x1B), ub(0x22),     // price = 4201250
            cb('N'), cb('S'), cb('D'), cb('Q')          // attribution
        };

        const auto result = decode_add_order_with_mpid_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.header.stock_locate == 2);
        assert(message.header.tracking_number == 3);
        assert(message.header.timestamp_ns == 2000);
        assert(message.order_reference_number == 101);
        assert(message.side == fgep::itch::Side::sell);
        assert(message.shares == 100);
        assert(wire::fixed_ascii_equals(message.stock, "MSFT"));
        assert(message.price == 4201250);
        assert(wire::fixed_ascii_equals(message.attribution, "NSDQ"));

        std::array<std::byte, length_add_order_with_mpid> encoded{};

        assert(
            encode_add_order_with_mpid_message(encoded, message)
            == ErrorCode::ok
        );
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    // -------------------------------------------------------------------------
    // E - Order Executed
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, length_order_executed> bytes{
            cb('E'),
            ub(0x00), ub(0x01),
            ub(0x00), ub(0x04),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x0B), ub(0xB8),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x64),     // order ref = 100
            ub(0x00), ub(0x00), ub(0x00), ub(0x32),     // executed shares = 50
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x23), ub(0x29)      // match = 9001
        };

        const auto result = decode_order_executed_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.header.stock_locate == 1);
        assert(message.header.tracking_number == 4);
        assert(message.header.timestamp_ns == 3000);
        assert(message.order_reference_number == 100);
        assert(message.executed_shares == 50);
        assert(message.match_number == 9001);

        std::array<std::byte, length_order_executed> encoded{};

        assert(
            encode_order_executed_message(encoded, message)
            == ErrorCode::ok
        );
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    // -------------------------------------------------------------------------
    // C - Order Executed with Price
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, length_order_executed_with_price> bytes{
            cb('C'),
            ub(0x00), ub(0x01),
            ub(0x00), ub(0x05),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x0F), ub(0xA0),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x64),     // order ref = 100
            ub(0x00), ub(0x00), ub(0x00), ub(0x19),     // executed shares = 25
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x23), ub(0x2A),     // match = 9002
            cb('Y'),                                    // printable
            ub(0x00), ub(0x1D), ub(0x05), ub(0xB0)      // price = 1902000
        };

        const auto result = decode_order_executed_with_price_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.header.stock_locate == 1);
        assert(message.header.tracking_number == 5);
        assert(message.header.timestamp_ns == 4000);
        assert(message.order_reference_number == 100);
        assert(message.executed_shares == 25);
        assert(message.match_number == 9002);
        assert(message.printable == Printable::printable);
        assert(message.execution_price == 1902000);

        std::array<std::byte, length_order_executed_with_price> encoded{};

        assert(
            encode_order_executed_with_price_message(encoded, message)
            == ErrorCode::ok
        );
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    // -------------------------------------------------------------------------
    // X - Order Cancel
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, length_order_cancel> bytes{
            cb('X'),
            ub(0x00), ub(0x01),
            ub(0x00), ub(0x06),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x13), ub(0x88),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x64),     // order ref = 100
            ub(0x00), ub(0x00), ub(0x00), ub(0x0A)      // canceled shares = 10
        };

        const auto result = decode_order_cancel_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.header.stock_locate == 1);
        assert(message.header.tracking_number == 6);
        assert(message.header.timestamp_ns == 5000);
        assert(message.order_reference_number == 100);
        assert(message.cancelled_shares == 10);

        std::array<std::byte, length_order_cancel> encoded{};

        assert(
            encode_order_cancel_message(encoded, message)
            == ErrorCode::ok
        );
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    // -------------------------------------------------------------------------
    // D - Order Delete
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, length_order_delete> bytes{
            cb('D'),
            ub(0x00), ub(0x01),
            ub(0x00), ub(0x07),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x17), ub(0x70),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x64)      // order ref = 100
        };

        const auto result = decode_order_delete_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.header.stock_locate == 1);
        assert(message.header.tracking_number == 7);
        assert(message.header.timestamp_ns == 6000);
        assert(message.order_reference_number == 100);

        std::array<std::byte, length_order_delete> encoded{};

        assert(
            encode_order_delete_message(encoded, message)
            == ErrorCode::ok
        );
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    // -------------------------------------------------------------------------
    // U - Order Replace
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, length_order_replace> bytes{
            cb('U'),
            ub(0x00), ub(0x01),
            ub(0x00), ub(0x08),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x1B), ub(0x58),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x64),     // original ref = 100
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0xC8),     // new ref = 200
            ub(0x00), ub(0x00), ub(0x01), ub(0x2C),     // shares = 300
            ub(0x00), ub(0x1D), ub(0x09), ub(0x98)      // price = 1903000
        };

        const auto result = decode_order_replace_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.header.stock_locate == 1);
        assert(message.header.tracking_number == 8);
        assert(message.header.timestamp_ns == 7000);
        assert(message.original_order_reference_number == 100);
        assert(message.new_order_reference_number == 200);
        assert(message.shares == 300);
        assert(message.price == 1903000);

        std::array<std::byte, length_order_replace> encoded{};

        assert(
            encode_order_replace_message(encoded, message)
            == ErrorCode::ok
        );
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    // -------------------------------------------------------------------------
    // P - Trade, Non-Cross
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, length_trade_non_cross> bytes{
            cb('P'),
            ub(0x00), ub(0x02),
            ub(0x00), ub(0x09),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x1F), ub(0x40),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x00), ub(0x65),     // order ref = 101
            cb('S'),                                    // sell
            ub(0x00), ub(0x00), ub(0x00), ub(0x64),     // shares = 100
            cb('M'), cb('S'), cb('F'), cb('T'), cb(' '), cb(' '), cb(' '), cb(' '),
            ub(0x00), ub(0x40), ub(0x1B), ub(0x22),     // price = 4201250
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x23), ub(0x2B)      // match = 9003
        };

        const auto result = decode_trade_non_cross_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.header.stock_locate == 2);
        assert(message.header.tracking_number == 9);
        assert(message.header.timestamp_ns == 8000);
        assert(message.order_reference_number == 101);
        assert(message.side == Side::sell);
        assert(message.shares == 100);
        assert(wire::fixed_ascii_equals(message.stock, "MSFT"));
        assert(message.price == 4201250);
        assert(message.match_number == 9003);

        std::array<std::byte, length_trade_non_cross> encoded{};

        assert(
            encode_trade_non_cross_message(encoded, message)
            == ErrorCode::ok
        );
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    // -------------------------------------------------------------------------
    // Q - Cross Trade
    // -------------------------------------------------------------------------
    //
    // Cross Trade uses an 8-byte shares field. This test intentionally uses a
    // value greater than uint32_t max to prove the parser keeps the full field.

    {
        constexpr std::uint64_t large_cross_shares = 0x0000000100000001ULL;

        const std::array<std::byte, length_cross_trade> bytes{
            cb('Q'),
            ub(0x00), ub(0x01),
            ub(0x00), ub(0x0A),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x23), ub(0x28),
            ub(0x00), ub(0x00), ub(0x00), ub(0x01),
            ub(0x00), ub(0x00), ub(0x00), ub(0x01),     // shares = 4294967297
            cb('A'), cb('A'), cb('P'), cb('L'), cb(' '), cb(' '), cb(' '), cb(' '),
            ub(0x00), ub(0x1C), ub(0xFD), ub(0xE0),     // cross price = 1900000
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x23), ub(0x2C),     // match = 9004
            cb('O')                                     // opening cross
        };

        const auto result = decode_cross_trade_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.header.stock_locate == 1);
        assert(message.header.tracking_number == 10);
        assert(message.header.timestamp_ns == 9000);
        assert(message.shares == large_cross_shares);
        assert(wire::fixed_ascii_equals(message.stock, "AAPL"));
        assert(message.cross_price == 1900000);
        assert(message.match_number == 9004);
        assert(message.cross_type == CrossType::opening_cross);

        std::array<std::byte, length_cross_trade> encoded{};

        assert(
            encode_cross_trade_message(encoded, message)
            == ErrorCode::ok
        );
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    // -------------------------------------------------------------------------
    // B - Broken Trade
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, length_broken_trade> bytes{
            cb('B'),
            ub(0x00), ub(0x01),
            ub(0x00), ub(0x0B),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x27), ub(0x10),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00), ub(0x23), ub(0x2B)      // match = 9003
        };

        const auto result = decode_broken_trade_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.header.stock_locate == 1);
        assert(message.header.tracking_number == 11);
        assert(message.header.timestamp_ns == 10000);
        assert(message.match_number == 9003);

        std::array<std::byte, length_broken_trade> encoded{};

        assert(
            encode_broken_trade_message(encoded, message)
            == ErrorCode::ok
        );
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    // -------------------------------------------------------------------------
    // Strict length and type checks
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, length_system_event + 1> too_long{
            cb('S'),
            ub(0x00), ub(0x00),
            ub(0x00), ub(0x01),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x01),
            cb('O'),
            ub(0x00)
        };

        const auto result = decode_system_event_message(too_long);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    {
        std::array<std::byte, length_system_event - 1> too_short{};

        const auto error = encode_system_event_message(
            too_short,
            SystemEventMessage{
                .header = Header{
                    .stock_locate = 0,
                    .tracking_number = 1,
                    .timestamp_ns = 1
                },
                .event_code = SystemEventCode::start_of_messages
            }
        );

        assert(error == ErrorCode::parse_error);
    }

    {
        const std::array<std::byte, length_system_event> wrong_type{
            cb('A'),
            ub(0x00), ub(0x00),
            ub(0x00), ub(0x01),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x00), ub(0x01),
            cb('O')
        };

        const auto result = decode_system_event_message(wrong_type);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    // -------------------------------------------------------------------------
    // Unknown and unsupported message dispatch
    // -------------------------------------------------------------------------

    {
        const std::array<std::byte, 1> unknown_message{
            cb('?')
        };

        const auto result = decode_message(unknown_message);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    {
        const std::array<std::byte, 1> unsupported_known_message{
            cb('R') // Stock Directory is known but not implemented yet.
        };

        const auto result = decode_message(unsupported_known_message);

        assert(result.failed());
        assert(result.error == ErrorCode::unsupported);
    }

    return 0;
}
