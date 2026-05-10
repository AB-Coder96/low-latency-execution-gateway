#include "fgep/instrument/instrument_directory.hpp"
#include "fgep/itch/itch_decode.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <variant>

namespace {

[[nodiscard]] constexpr std::byte ub(unsigned int value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

[[nodiscard]] constexpr std::byte cb(char value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

} // namespace

int main() {
    using namespace fgep;

    instrument::InstrumentDirectory directory{};

    const std::array<std::byte, itch::length_stock_directory> stock_directory_bytes{
        cb('R'),

        ub(0x00), ub(0x01),                         // stock locate = 1
        ub(0x00), ub(0x02),                         // tracking number = 2

        ub(0x00), ub(0x00), ub(0x00),
        ub(0x00), ub(0x03), ub(0xE8),               // timestamp = 1000

        cb('A'), cb('A'), cb('P'), cb('L'),
        cb(' '), cb(' '), cb(' '), cb(' '),         // stock = AAPL

        cb('Q'),                                    // market category
        cb('N'),                                    // financial status

        ub(0x00), ub(0x00), ub(0x00), ub(0x64),     // round lot = 100

        cb('N'),                                    // round lots only
        cb('C'),                                    // issue classification

        cb(' '), cb(' '),                           // issue subtype

        cb('P'),                                    // authenticity
        cb('N'),                                    // short sale threshold
        cb('N'),                                    // IPO flag
        cb('1'),                                    // LULD tier
        cb('N'),                                    // ETP flag

        ub(0x00), ub(0x00), ub(0x00), ub(0x00),     // ETP leverage factor

        cb('N')                                     // inverse indicator
    };

    const auto decoded_directory_message =
        itch::decode_message(stock_directory_bytes);

    assert(decoded_directory_message.ok());

    const auto* stock_directory =
        std::get_if<itch::StockDirectoryMessage>(
            &decoded_directory_message.value
        );

    assert(stock_directory != nullptr);

    assert(
        directory.apply(venue::VenueId{1}, *stock_directory)
        == ErrorCode::ok
    );

    const auto* metadata = directory.find(
        venue::VenueId{1},
        itch::StockLocate{1}
    );

    assert(metadata != nullptr);
    assert(metadata->key.venue_id == 1);
    assert(metadata->key.stock_locate == 1);
    assert(wire::fixed_ascii_equals(metadata->stock, "AAPL"));
    assert(metadata->round_lot_size == 100);
    assert(metadata->trading_status == instrument::SymbolTradingStatus::unknown);

    const std::array<std::byte, itch::length_stock_trading_action>
        trading_action_bytes{
            cb('H'),

            ub(0x00), ub(0x01),                     // stock locate = 1
            ub(0x00), ub(0x03),                     // tracking number = 3

            ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x07), ub(0xD0),           // timestamp = 2000

            cb('A'), cb('A'), cb('P'), cb('L'),
            cb(' '), cb(' '), cb(' '), cb(' '),     // stock = AAPL

            cb('T'),                                // trading state = trading
            cb(' '),                                // reserved

            cb('T'), cb('1'), cb(' '), cb(' ')      // reason = T1
        };

    const auto decoded_trading_action =
        itch::decode_message(trading_action_bytes);

    assert(decoded_trading_action.ok());

    const auto* trading_action =
        std::get_if<itch::StockTradingActionMessage>(
            &decoded_trading_action.value
        );

    assert(trading_action != nullptr);

    assert(
        directory.apply(venue::VenueId{1}, *trading_action)
        == ErrorCode::ok
    );

    metadata = directory.find(venue::VenueId{1}, itch::StockLocate{1});

    assert(metadata != nullptr);
    assert(metadata->trading_status == instrument::SymbolTradingStatus::trading);
    assert(wire::fixed_ascii_equals(metadata->last_trading_action_reason, "T1"));

    auto halted_bytes = trading_action_bytes;
    halted_bytes[19] = cb('H');
    halted_bytes[21] = cb('H');
    halted_bytes[22] = cb('1');
    halted_bytes[23] = cb('0');
    halted_bytes[24] = cb(' ');

    const auto decoded_halt = itch::decode_message(halted_bytes);

    assert(decoded_halt.ok());

    const auto* halt_action =
        std::get_if<itch::StockTradingActionMessage>(&decoded_halt.value);

    assert(halt_action != nullptr);

    assert(
        directory.apply(venue::VenueId{1}, *halt_action)
        == ErrorCode::ok
    );

    metadata = directory.find(venue::VenueId{1}, itch::StockLocate{1});

    assert(metadata != nullptr);
    assert(metadata->trading_status == instrument::SymbolTradingStatus::halted);
    assert(wire::fixed_ascii_equals(metadata->last_trading_action_reason, "H10"));

    const std::array<std::byte, itch::length_stock_trading_action>
        missing_symbol_action_bytes{
            cb('H'),

            ub(0x00), ub(0x63),                     // stock locate = 99
            ub(0x00), ub(0x04),                     // tracking number = 4

            ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x0B), ub(0xB8),           // timestamp = 3000

            cb('M'), cb('S'), cb('F'), cb('T'),
            cb(' '), cb(' '), cb(' '), cb(' '),     // stock = MSFT

            cb('T'),                                // trading state
            cb(' '),                                // reserved

            cb(' '), cb(' '), cb(' '), cb(' ')      // reason
        };

    const auto decoded_missing_action =
        itch::decode_message(missing_symbol_action_bytes);

    assert(decoded_missing_action.ok());

    const auto* missing_action =
        std::get_if<itch::StockTradingActionMessage>(
            &decoded_missing_action.value
        );

    assert(missing_action != nullptr);

    assert(
        directory.apply(venue::VenueId{1}, *missing_action)
        == ErrorCode::not_found
    );

    return 0;
}