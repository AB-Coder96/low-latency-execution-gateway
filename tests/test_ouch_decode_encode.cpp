#include "fgep/ouch/ouch_decode.hpp"
#include "fgep/ouch/ouch_encode.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <span>

namespace {

[[nodiscard]] constexpr std::byte ub(unsigned int value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

[[nodiscard]] constexpr std::byte cb(char value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

template <std::size_t N>
void assert_round_trip_message(const std::array<std::byte, N>& input) {
    const auto decoded = fgep::ouch::decode_message(
        std::span<const std::byte>{input}
    );

    assert(decoded.ok());

    std::array<std::byte, N> output{};

    const auto encode_error = fgep::ouch::encode_message(
        std::span<std::byte>{output},
        decoded.value
    );

    assert(encode_error == fgep::ErrorCode::ok);
    assert(output == input);
}

} // namespace

int main() {
    using namespace fgep;
    using namespace fgep::ouch;

    {
        const std::array<std::byte, length_enter_order_base> bytes{
            cb('O'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x01),
            cb('B'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x64),
            cb('A'), cb('A'), cb('P'), cb('L'), cb(' '), cb(' '), cb(' '), cb(' '),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x1D), ub(0x07), ub(0xA4),
            cb('0'),
            cb('Y'),
            cb('A'),
            cb('N'),
            cb('N'),
            cb('C'), cb('L'), cb('O'), cb('R'), cb('D'), cb('1'), cb('2'),
            cb('3'), cb(' '), cb(' '), cb(' '), cb(' '), cb(' '), cb(' '),
            ub(0x00), ub(0x00)
        };

        const auto result = decode_enter_order_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.user_ref_num == 1);
        assert(message.side == fgep::ouch::Side::buy);
        assert(message.quantity == 100);
        assert(wire::fixed_ascii_equals(message.symbol, "AAPL"));
        assert(message.price == 1902500);
        assert(message.time_in_force == TimeInForce::day);
        assert(message.display == Display::visible);
        assert(message.capacity == Capacity::agency);
        assert(
            message.intermarket_sweep_eligibility
            == IntermarketSweepEligibility::not_eligible
        );
        assert(message.cross_type == CrossType::continuous_market);
        assert(wire::fixed_ascii_equals(message.cl_ord_id, "CLORD123"));
        assert(message.optional_appendage.empty());

        std::array<std::byte, length_enter_order_base> encoded{};

        assert(encode_enter_order_message(encoded, message) == ErrorCode::ok);
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    {
        const std::array<std::byte, length_replace_order_base> bytes{
            cb('U'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x01),
            ub(0x00), ub(0x00), ub(0x00), ub(0x02),
            ub(0x00), ub(0x00), ub(0x00), ub(0x96),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x1D), ub(0x11), ub(0x8C),
            cb('0'),
            cb('Y'),
            cb('N'),
            cb('R'), cb('E'), cb('P'), cb('L'), cb('1'), cb('2'), cb('3'),
            cb(' '), cb(' '), cb(' '), cb(' '), cb(' '), cb(' '), cb(' '),
            ub(0x00), ub(0x00)
        };

        const auto result = decode_replace_order_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.orig_user_ref_num == 1);
        assert(message.user_ref_num == 2);
        assert(message.quantity == 150);
        assert(message.price == 1905036);
        assert(message.time_in_force == TimeInForce::day);
        assert(message.display == Display::visible);
        assert(
            message.intermarket_sweep_eligibility
            == IntermarketSweepEligibility::not_eligible
        );
        assert(wire::fixed_ascii_equals(message.cl_ord_id, "REPL123"));
        assert(message.optional_appendage.empty());

        std::array<std::byte, length_replace_order_base> encoded{};

        assert(encode_replace_order_message(encoded, message) == ErrorCode::ok);
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    {
        const std::array<std::byte, length_cancel_order_base> bytes{
            cb('X'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x02),
            ub(0x00), ub(0x00), ub(0x00), ub(0x00),
            ub(0x00), ub(0x00)
        };

        const auto result = decode_cancel_order_message(bytes);

        assert(result.ok());

        const auto& message = result.value;

        assert(message.user_ref_num == 2);
        assert(message.quantity == 0);
        assert(message.optional_appendage.empty());

        std::array<std::byte, length_cancel_order_base> encoded{};

        assert(encode_cancel_order_message(encoded, message) == ErrorCode::ok);
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    {
        const std::array<std::byte, length_cancel_order_base + 3> bytes{
            cb('X'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x03),
            ub(0x00), ub(0x00), ub(0x00), ub(0x32),
            ub(0x00), ub(0x03),
            ub(0x1C), ub(0x01), ub(0x07)
        };

        const auto result = decode_cancel_order_message(bytes);

        assert(result.ok());
        assert(result.value.optional_appendage.size() == 3);
        assert(result.value.optional_appendage[0] == ub(0x1C));
        assert(result.value.optional_appendage[1] == ub(0x01));
        assert(result.value.optional_appendage[2] == ub(0x07));

        std::array<std::byte, length_cancel_order_base + 3> encoded{};

        assert(encode_cancel_order_message(encoded, result.value) == ErrorCode::ok);
        assert(encoded == bytes);

        assert_round_trip_message(bytes);
    }

    {
        const std::array<std::byte, length_cancel_order_base + 1> bytes{
            cb('X'),
            ub(0x00), ub(0x00), ub(0x00), ub(0x03),
            ub(0x00), ub(0x00), ub(0x00), ub(0x32),
            ub(0x00), ub(0x02),
            ub(0x1C)
        };

        const auto result = decode_cancel_order_message(bytes);

        assert(result.failed());
        assert(result.error == ErrorCode::parse_error);
    }

    return 0;
}