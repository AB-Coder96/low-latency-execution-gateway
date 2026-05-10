#include "fgep/bbo/normalized_bbo.hpp"
#include "fgep/book/multi_venue_book.hpp"
#include "fgep/instrument/instrument_directory.hpp"
#include "fgep/replay/itch_replay_apply.hpp"
#include "fgep/replay/moldudp64_itch_replay.hpp"
#include "fgep/wire/fixed_ascii.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] constexpr std::byte ub(unsigned int value) noexcept {
    return static_cast<std::byte>(static_cast<unsigned char>(value));
}

void append_u16_be(std::vector<std::byte>& bytes, std::uint16_t value) {
    bytes.push_back(ub((value >> 8U) & 0xFFU));
    bytes.push_back(ub(value & 0xFFU));
}

void append_u32_be(std::vector<std::byte>& bytes, std::uint32_t value) {
    bytes.push_back(ub((value >> 24U) & 0xFFU));
    bytes.push_back(ub((value >> 16U) & 0xFFU));
    bytes.push_back(ub((value >> 8U) & 0xFFU));
    bytes.push_back(ub(value & 0xFFU));
}

void append_u48_be(std::vector<std::byte>& bytes, std::uint64_t value) {
    bytes.push_back(ub((value >> 40U) & 0xFFU));
    bytes.push_back(ub((value >> 32U) & 0xFFU));
    bytes.push_back(ub((value >> 24U) & 0xFFU));
    bytes.push_back(ub((value >> 16U) & 0xFFU));
    bytes.push_back(ub((value >> 8U) & 0xFFU));
    bytes.push_back(ub(value & 0xFFU));
}

void append_u64_be(std::vector<std::byte>& bytes, std::uint64_t value) {
    bytes.push_back(ub((value >> 56U) & 0xFFU));
    bytes.push_back(ub((value >> 48U) & 0xFFU));
    bytes.push_back(ub((value >> 40U) & 0xFFU));
    bytes.push_back(ub((value >> 32U) & 0xFFU));
    bytes.push_back(ub((value >> 24U) & 0xFFU));
    bytes.push_back(ub((value >> 16U) & 0xFFU));
    bytes.push_back(ub((value >> 8U) & 0xFFU));
    bytes.push_back(ub(value & 0xFFU));
}

void append_char(std::vector<std::byte>& bytes, char value) {
    bytes.push_back(static_cast<std::byte>(
        static_cast<unsigned char>(value)
    ));
}

void append_fixed_ascii(
    std::vector<std::byte>& bytes,
    std::string_view text,
    std::size_t width
) {
    for (std::size_t index = 0; index < width; ++index) {
        const char value = index < text.size() ? text[index] : ' ';
        append_char(bytes, value);
    }
}

void append_itch_header(
    std::vector<std::byte>& bytes,
    char message_type,
    fgep::itch::StockLocate stock_locate,
    fgep::itch::TrackingNumber tracking_number,
    fgep::itch::TimestampNs timestamp_ns
) {
    append_char(bytes, message_type);
    append_u16_be(bytes, stock_locate);
    append_u16_be(bytes, tracking_number);
    append_u48_be(bytes, timestamp_ns);
}

[[nodiscard]] std::vector<std::byte> stock_directory_message(
    fgep::itch::StockLocate stock_locate,
    fgep::itch::TrackingNumber tracking_number,
    fgep::itch::TimestampNs timestamp_ns,
    std::string_view stock
) {
    std::vector<std::byte> bytes{};

    append_itch_header(
        bytes,
        'R',
        stock_locate,
        tracking_number,
        timestamp_ns
    );

    append_fixed_ascii(bytes, stock, 8);
    append_char(bytes, 'Q');
    append_char(bytes, 'N');
    append_u32_be(bytes, 100);
    append_char(bytes, 'N');
    append_char(bytes, 'C');
    append_fixed_ascii(bytes, "", 2);
    append_char(bytes, 'P');
    append_char(bytes, 'N');
    append_char(bytes, 'N');
    append_char(bytes, '1');
    append_char(bytes, 'N');
    append_u32_be(bytes, 0);
    append_char(bytes, 'N');

    assert(bytes.size() == fgep::itch::length_stock_directory);

    return bytes;
}

[[nodiscard]] std::vector<std::byte> trading_action_message(
    fgep::itch::StockLocate stock_locate,
    fgep::itch::TrackingNumber tracking_number,
    fgep::itch::TimestampNs timestamp_ns,
    std::string_view stock,
    char trading_state,
    std::string_view reason
) {
    std::vector<std::byte> bytes{};

    append_itch_header(
        bytes,
        'H',
        stock_locate,
        tracking_number,
        timestamp_ns
    );

    append_fixed_ascii(bytes, stock, 8);
    append_char(bytes, trading_state);
    append_char(bytes, ' ');
    append_fixed_ascii(bytes, reason, 4);

    assert(bytes.size() == fgep::itch::length_stock_trading_action);

    return bytes;
}

[[nodiscard]] std::vector<std::byte> add_order_message(
    fgep::itch::StockLocate stock_locate,
    fgep::itch::TrackingNumber tracking_number,
    fgep::itch::TimestampNs timestamp_ns,
    fgep::itch::OrderReferenceNumber order_reference_number,
    char side,
    fgep::itch::Shares shares,
    std::string_view stock,
    fgep::itch::Price4 price
) {
    std::vector<std::byte> bytes{};

    append_itch_header(
        bytes,
        'A',
        stock_locate,
        tracking_number,
        timestamp_ns
    );

    append_u64_be(bytes, order_reference_number);
    append_char(bytes, side);
    append_u32_be(bytes, shares);
    append_fixed_ascii(bytes, stock, 8);
    append_u32_be(bytes, price);

    assert(bytes.size() == fgep::itch::length_add_order_no_mpid);

    return bytes;
}

void append_mold_message_block(
    std::vector<std::byte>& packet,
    const std::vector<std::byte>& message
) {
    assert(message.size() <= 0xFFFFU);

    append_u16_be(
        packet,
        static_cast<std::uint16_t>(message.size())
    );

    packet.insert(packet.end(), message.begin(), message.end());
}

[[nodiscard]] std::vector<std::byte> moldudp64_packet(
    std::string_view session,
    fgep::moldudp64::SequenceNumber first_sequence_number,
    const std::vector<std::vector<std::byte>>& messages
) {
    std::vector<std::byte> packet{};

    append_fixed_ascii(packet, session, 10);
    append_u64_be(packet, first_sequence_number);
    append_u16_be(
        packet,
        static_cast<fgep::moldudp64::MessageCount>(messages.size())
    );

    for (const auto& message : messages) {
        append_mold_message_block(packet, message);
    }

    return packet;
}

[[nodiscard]] fgep::instrument::CanonicalSymbol canonical_symbol(
    const char* text
) {
    const auto stock = fgep::wire::make_fixed_ascii<8>(text);
    assert(stock.ok());

    return fgep::instrument::CanonicalSymbol{stock.value};
}

} // namespace

int main() {
    using namespace fgep;

    instrument::InstrumentDirectory directory{};
    book::MultiVenueBook books{directory};

    assert(books.add_venue(venue::VenueId{1}) == ErrorCode::ok);
    assert(books.add_venue(venue::VenueId{2}) == ErrorCode::ok);

    const auto venue_one_packet_bytes = moldudp64_packet(
        "ITCHVEN001",
        1000,
        {
            stock_directory_message(1, 1, 1000, "AAPL"),
            trading_action_message(1, 2, 2000, "AAPL", 'T', "T1"),
            add_order_message(
                1,
                3,
                3000,
                100,
                'B',
                100,
                "AAPL",
                1902500
            )
        }
    );

    const auto venue_one_packet =
        replay::decode_moldudp64_itch_packet(venue_one_packet_bytes);

    assert(venue_one_packet.ok());

    replay::ItchReplayApplier venue_one_applier{
        venue::VenueId{1},
        directory,
        books
    };

    assert(venue_one_applier.apply(venue_one_packet.value) == ErrorCode::ok);

    const auto venue_two_packet_bytes = moldudp64_packet(
        "ITCHVEN002",
        2000,
        {
            stock_directory_message(7, 1, 1000, "AAPL"),
            trading_action_message(7, 2, 2000, "AAPL", 'T', "T1"),
            add_order_message(
                7,
                3,
                3000,
                200,
                'S',
                50,
                "AAPL",
                1903500
            )
        }
    );

    const auto venue_two_packet =
        replay::decode_moldudp64_itch_packet(venue_two_packet_bytes);

    assert(venue_two_packet.ok());

    replay::ItchReplayApplier venue_two_applier{
        venue::VenueId{2},
        directory,
        books
    };

    assert(venue_two_applier.apply(venue_two_packet.value) == ErrorCode::ok);

    const auto* venue_one_metadata = directory.find(
        venue::VenueId{1},
        itch::StockLocate{1}
    );

    assert(venue_one_metadata != nullptr);
    assert(venue_one_metadata->trading_status
        == instrument::SymbolTradingStatus::trading);

    const auto* venue_two_metadata = directory.find(
        venue::VenueId{2},
        itch::StockLocate{7}
    );

    assert(venue_two_metadata != nullptr);
    assert(venue_two_metadata->trading_status
        == instrument::SymbolTradingStatus::trading);

    const auto aapl = canonical_symbol("AAPL");

    bbo::NormalizedBboView bbo_view{books};

    const auto bbo = bbo_view.get(aapl);

    assert(bbo.bid.has_value());
    assert(bbo.bid->key.venue_id == 1);
    assert(bbo.bid->key.stock_locate == 1);
    assert(bbo.bid->price == 1902500);
    assert(bbo.bid->quantity == 100);

    assert(bbo.ask.has_value());
    assert(bbo.ask->key.venue_id == 2);
    assert(bbo.ask->key.stock_locate == 7);
    assert(bbo.ask->price == 1903500);
    assert(bbo.ask->quantity == 50);

    const auto& venue_one_stats = venue_one_applier.stats();

    assert(venue_one_stats.packets == 1);
    assert(venue_one_stats.data_packets == 1);
    assert(venue_one_stats.instrument_messages == 2);
    assert(venue_one_stats.book_messages == 1);
    assert(venue_one_stats.ignored_messages == 0);
    assert(venue_one_stats.failed_messages == 0);
    assert(venue_one_stats.has_last_sequence_number);
    assert(venue_one_stats.last_sequence_number == 1002);

    const auto& venue_two_stats = venue_two_applier.stats();

    assert(venue_two_stats.packets == 1);
    assert(venue_two_stats.data_packets == 1);
    assert(venue_two_stats.instrument_messages == 2);
    assert(venue_two_stats.book_messages == 1);
    assert(venue_two_stats.ignored_messages == 0);
    assert(venue_two_stats.failed_messages == 0);
    assert(venue_two_stats.has_last_sequence_number);
    assert(venue_two_stats.last_sequence_number == 2002);

    return 0;
}