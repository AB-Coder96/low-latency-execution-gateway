#pragma once

#include "fgep/book/multi_venue_book.hpp"
#include "fgep/core/errors.hpp"
#include "fgep/instrument/instrument_directory.hpp"
#include "fgep/replay/moldudp64_itch_replay.hpp"
#include "fgep/venue/venue.hpp"

#include <cstddef>

namespace fgep::replay {

struct ItchReplayApplyStats {
    std::size_t packets{};
    std::size_t data_packets{};
    std::size_t heartbeat_packets{};
    std::size_t end_of_session_packets{};

    std::size_t instrument_messages{};
    std::size_t book_messages{};
    std::size_t ignored_messages{};
    std::size_t failed_messages{};

    bool has_last_sequence_number{};
    moldudp64::SequenceNumber last_sequence_number{};
};

class ItchReplayApplier {
public:
    ItchReplayApplier(
        venue::VenueId venue_id,
        instrument::InstrumentDirectory& directory,
        book::MultiVenueBook& books
    );

    [[nodiscard]] ErrorCode apply(
        const MoldUdp64ItchReplayPacket& packet
    );

    [[nodiscard]] ErrorCode apply(
        const DecodedItchMessage& decoded_message
    );

    [[nodiscard]] const ItchReplayApplyStats& stats() const noexcept;

private:
    [[nodiscard]] ErrorCode apply_message(const itch::Message& message);

    venue::VenueId venue_id_{};
    instrument::InstrumentDirectory* directory_{};
    book::MultiVenueBook* books_{};
    ItchReplayApplyStats stats_{};
};

} // namespace fgep::replay