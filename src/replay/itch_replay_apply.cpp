#include "fgep/replay/itch_replay_apply.hpp"

#include <type_traits>
#include <variant>

namespace fgep::replay {

ItchReplayApplier::ItchReplayApplier(
    venue::VenueId venue_id,
    instrument::InstrumentDirectory& directory,
    book::MultiVenueBook& books
)
    : venue_id_{venue_id},
      directory_{&directory},
      books_{&books} {
}

ErrorCode ItchReplayApplier::apply(
    const MoldUdp64ItchReplayPacket& packet
) {
    if (!venue::is_valid_venue_id(venue_id_)
        || directory_ == nullptr
        || books_ == nullptr) {
        return ErrorCode::invalid_state;
    }

    ++stats_.packets;

    switch (packet.kind) {
        case ReplayPacketKind::data:
            ++stats_.data_packets;
            break;

        case ReplayPacketKind::heartbeat:
            ++stats_.heartbeat_packets;
            return ErrorCode::ok;

        case ReplayPacketKind::end_of_session:
            ++stats_.end_of_session_packets;
            return ErrorCode::ok;
    }

    for (const auto& decoded_message : packet.messages) {
        const auto error = apply(decoded_message);

        if (error != ErrorCode::ok) {
            ++stats_.failed_messages;
            return error;
        }
    }

    return ErrorCode::ok;
}

ErrorCode ItchReplayApplier::apply(
    const DecodedItchMessage& decoded_message
) {
    stats_.has_last_sequence_number = true;
    stats_.last_sequence_number = decoded_message.sequence_number;

    return apply_message(decoded_message.message);
}

const ItchReplayApplyStats& ItchReplayApplier::stats() const noexcept {
    return stats_;
}

ErrorCode ItchReplayApplier::apply_message(const itch::Message& message) {
    return std::visit(
        [this](const auto& concrete_message) -> ErrorCode {
            using MessageT = std::decay_t<decltype(concrete_message)>;

            if constexpr (
                std::is_same_v<MessageT, itch::StockDirectoryMessage>
            ) {
                const auto error = directory_->apply(
                    venue_id_,
                    concrete_message
                );

                if (error == ErrorCode::ok) {
                    ++stats_.instrument_messages;
                }

                return error;
            } else if constexpr (
                std::is_same_v<MessageT, itch::StockTradingActionMessage>
            ) {
                const auto error = directory_->apply(
                    venue_id_,
                    concrete_message
                );

                if (error == ErrorCode::ok) {
                    ++stats_.instrument_messages;
                }

                return error;
            } else if constexpr (
                std::is_same_v<MessageT, itch::AddOrderNoMpidMessage>
                || std::is_same_v<MessageT, itch::AddOrderWithMpidMessage>
                || std::is_same_v<MessageT, itch::OrderExecutedMessage>
                || std::is_same_v<
                    MessageT,
                    itch::OrderExecutedWithPriceMessage
                >
                || std::is_same_v<MessageT, itch::OrderCancelMessage>
                || std::is_same_v<MessageT, itch::OrderDeleteMessage>
                || std::is_same_v<MessageT, itch::OrderReplaceMessage>
            ) {
                const auto error = books_->apply(
                    venue_id_,
                    itch::Message{concrete_message}
                );

                if (error == ErrorCode::ok) {
                    ++stats_.book_messages;
                }

                return error;
            } else {
                ++stats_.ignored_messages;
                return ErrorCode::ok;
            }
        },
        message
    );
}

} // namespace fgep::replay