#pragma once

#include "fgep/book/book_types.hpp"
#include "fgep/core/errors.hpp"
#include "fgep/instrument/instrument_key.hpp"
#include "fgep/itch/itch_wire_messages.hpp"

#include <functional>
#include <map>
#include <optional>
#include <unordered_map>

namespace fgep::book {

class SymbolOrderBook {
public:
    SymbolOrderBook(
        instrument::InstrumentKey key,
        itch::StockSymbol stock
    );

    [[nodiscard]] const instrument::InstrumentKey& key() const noexcept;
    [[nodiscard]] const itch::StockSymbol& stock() const noexcept;

    [[nodiscard]] ErrorCode apply(const itch::Message& message);

    [[nodiscard]] ErrorCode apply(const itch::SystemEventMessage& message);
    [[nodiscard]] ErrorCode apply(const itch::StockDirectoryMessage& message);
    [[nodiscard]] ErrorCode apply(
        const itch::StockTradingActionMessage& message
    );
    [[nodiscard]] ErrorCode apply(const itch::AddOrderNoMpidMessage& message);
    [[nodiscard]] ErrorCode apply(const itch::AddOrderWithMpidMessage& message);
    [[nodiscard]] ErrorCode apply(const itch::OrderExecutedMessage& message);
    [[nodiscard]] ErrorCode apply(
        const itch::OrderExecutedWithPriceMessage& message
    );
    [[nodiscard]] ErrorCode apply(const itch::OrderCancelMessage& message);
    [[nodiscard]] ErrorCode apply(const itch::OrderDeleteMessage& message);
    [[nodiscard]] ErrorCode apply(const itch::OrderReplaceMessage& message);
    [[nodiscard]] ErrorCode apply(const itch::TradeNonCrossMessage& message);
    [[nodiscard]] ErrorCode apply(const itch::CrossTradeMessage& message);
    [[nodiscard]] ErrorCode apply(const itch::BrokenTradeMessage& message);

    [[nodiscard]] std::optional<Level> best_bid() const noexcept;
    [[nodiscard]] std::optional<Level> best_ask() const noexcept;

    [[nodiscard]] bool contains_order(
        itch::OrderReferenceNumber order_reference_number
    ) const noexcept;

    [[nodiscard]] std::size_t order_count() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    struct RestingOrder {
        itch::OrderReferenceNumber order_reference_number{};
        itch::Side side{itch::Side::buy};
        itch::Shares shares{};
        itch::Price4 price{};
    };

    struct LevelState {
        Quantity quantity{};
        std::size_t order_count{};
    };

    using BidLevels =
        std::map<itch::Price4, LevelState, std::greater<itch::Price4>>;

    using AskLevels =
        std::map<itch::Price4, LevelState, std::less<itch::Price4>>;

    [[nodiscard]] ErrorCode require_matching_header(
        const itch::Header& header
    ) const noexcept;

    [[nodiscard]] ErrorCode require_matching_stock(
        const itch::StockSymbol& stock
    ) const noexcept;

    [[nodiscard]] ErrorCode add_order(
        itch::OrderReferenceNumber order_reference_number,
        itch::Side side,
        itch::Shares shares,
        const itch::StockSymbol& stock,
        itch::Price4 price
    );

    [[nodiscard]] ErrorCode reduce_order(
        itch::OrderReferenceNumber order_reference_number,
        itch::Shares shares_to_reduce
    );

    [[nodiscard]] ErrorCode delete_order(
        itch::OrderReferenceNumber order_reference_number
    );

    void add_to_level(const RestingOrder& order);

    void remove_from_level(
        const RestingOrder& order,
        itch::Shares shares
    );

    [[nodiscard]] static Level make_level(
        itch::Price4 price,
        const LevelState& state
    ) noexcept;

    instrument::InstrumentKey key_{};
    itch::StockSymbol stock_{};

    std::unordered_map<itch::OrderReferenceNumber, RestingOrder> orders_{};
    BidLevels bids_{};
    AskLevels asks_{};
};

} // namespace fgep::book