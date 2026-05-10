#include "fgep/itch/itch_decode.hpp"
#include "fgep/itch/itch_encode.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <span>
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

    const std::array<std::byte, itch::length_stock_trading_action> bytes{
        cb('H'),

        ub(0x00), ub(0x01),                         // stock locate = 1
        ub(0x00), ub(0x03),                         // tracking number = 3

        ub(0x00), ub(0x00), ub(0x00),
        ub(0x00), ub(0x07), ub(0xD0),               // timestamp = 2000

        cb('A'), cb('A'), cb('P'), cb('L'),
        cb(' '), cb(' '), cb(' '), cb(' '),         // stock = AAPL

        cb('T'),                                    // trading state
        cb(' '),                                    // reserved

        cb('T'), cb('1'), cb(' '), cb(' ')          // reason = T1
    };

    const auto decoded = itch::decode_stock_trading_action_message(bytes);

    assert(decoded.ok());

    const auto& message = decoded.value;

    assert(message.header.stock_locate == 1);
    assert(message.header.tracking_number == 3);
    assert(message.header.timestamp_ns == 2000);
    assert(wire::fixed_ascii_equals(message.stock, "AAPL"));
    assert(message.trading_state == itch::TradingState::trading);
    assert(message.reserved == ' ');
    assert(wire::fixed_ascii_equals(message.reason, "T1"));

    std::array<std::byte, itch::length_stock_trading_action> encoded{};

    assert(
        itch::encode_stock_trading_action_message(encoded, message)
        == ErrorCode::ok
    );

    assert(encoded == bytes);

    const auto generic_decoded = itch::decode_message(bytes);

    assert(generic_decoded.ok());

    const auto* generic_message =
        std::get_if<itch::StockTradingActionMessage>(&generic_decoded.value);

    assert(generic_message != nullptr);
    assert(generic_message->trading_state == itch::TradingState::trading);

    std::array<std::byte, itch::length_stock_trading_action> generic_encoded{};

    assert(
        itch::encode_message(generic_encoded, generic_decoded.value)
        == ErrorCode::ok
    );

    assert(generic_encoded == bytes);

    {
        auto halted = bytes;
        halted[19] = cb('H');

        const auto result = itch::decode_stock_trading_action_message(halted);

        assert(result.ok());
        assert(result.value.trading_state == itch::TradingState::halted);
    }

    {
        auto paused = bytes;
        paused[19] = cb('P');

        const auto result = itch::decode_stock_trading_action_message(paused);

        assert(result.ok());
        assert(result.value.trading_state == itch::TradingState::paused);
    }

    {
        auto quote_only = bytes;
        quote_only[19] = cb('Q');

        const auto result =
            itch::decode_stock_trading_action_message(quote_only);

        assert(result.ok());
        assert(
            result.value.trading_state
            == itch::TradingState::quotation_only
        );
    }

    {
        auto bad_state = bytes;
        bad_state[19] = cb('?');

        const auto result =
            itch::decode_stock_trading_action_message(bad_state);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    return 0;
}