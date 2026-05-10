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

    const std::array<std::byte, itch::length_stock_directory> bytes{
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

    const auto decoded = itch::decode_stock_directory_message(bytes);

    assert(decoded.ok());

    const auto& message = decoded.value;

    assert(message.header.stock_locate == 1);
    assert(message.header.tracking_number == 2);
    assert(message.header.timestamp_ns == 1000);
    assert(wire::fixed_ascii_equals(message.stock, "AAPL"));
    assert(message.market_category == 'Q');
    assert(message.financial_status_indicator == 'N');
    assert(message.round_lot_size == 100);
    assert(message.round_lots_only == 'N');
    assert(message.issue_classification == 'C');
    assert(wire::fixed_ascii_equals(message.issue_sub_type, ""));
    assert(message.authenticity == 'P');
    assert(message.short_sale_threshold_indicator == 'N');
    assert(message.ipo_flag == 'N');
    assert(message.luld_reference_price_tier == '1');
    assert(message.etp_flag == 'N');
    assert(message.etp_leverage_factor == 0);
    assert(message.inverse_indicator == 'N');

    std::array<std::byte, itch::length_stock_directory> encoded{};

    assert(itch::encode_stock_directory_message(encoded, message) == ErrorCode::ok);
    assert(encoded == bytes);

    const auto generic_decoded = itch::decode_message(bytes);

    assert(generic_decoded.ok());

    const auto* generic_message =
        std::get_if<itch::StockDirectoryMessage>(&generic_decoded.value);

    assert(generic_message != nullptr);
    assert(wire::fixed_ascii_equals(generic_message->stock, "AAPL"));

    std::array<std::byte, itch::length_stock_directory> generic_encoded{};

    assert(
        itch::encode_message(generic_encoded, generic_decoded.value)
        == ErrorCode::ok
    );

    assert(generic_encoded == bytes);

    {
        auto bad_type = bytes;
        bad_type[0] = cb('H');

        const auto result = itch::decode_stock_directory_message(bad_type);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    {
        std::array<std::byte, itch::length_stock_directory - 1> short_bytes{};

        const auto result = itch::decode_stock_directory_message(short_bytes);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    return 0;
}