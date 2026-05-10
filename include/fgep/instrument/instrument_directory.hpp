#pragma once

#include "fgep/core/errors.hpp"
#include "fgep/instrument/instrument_key.hpp"
#include "fgep/itch/itch_wire_messages.hpp"
#include "fgep/venue/venue.hpp"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace fgep::instrument {

enum class SymbolTradingStatus {
    unknown,
    halted,
    paused,
    quotation_only,
    trading
};

struct InstrumentMetadata {
    InstrumentKey key{};
    itch::StockSymbol stock{};

    char market_category{' '};
    char financial_status_indicator{' '};
    itch::RoundLotSize round_lot_size{};
    char round_lots_only{'N'};
    char issue_classification{' '};
    itch::IssueSubType issue_sub_type{};
    char authenticity{' '};
    char short_sale_threshold_indicator{' '};
    char ipo_flag{' '};
    char luld_reference_price_tier{' '};
    char etp_flag{' '};
    itch::EtpLeverageFactor etp_leverage_factor{};
    char inverse_indicator{' '};

    SymbolTradingStatus trading_status{SymbolTradingStatus::unknown};
    itch::ReasonCode4 last_trading_action_reason{};
};

class InstrumentDirectory {
public:
    [[nodiscard]] ErrorCode apply(
        venue::VenueId venue_id,
        const itch::StockDirectoryMessage& message
    );

    [[nodiscard]] ErrorCode apply(
        venue::VenueId venue_id,
        const itch::StockTradingActionMessage& message
    );

    [[nodiscard]] const InstrumentMetadata* find(
        const InstrumentKey& key
    ) const noexcept;

    [[nodiscard]] const InstrumentMetadata* find(
        venue::VenueId venue_id,
        itch::StockLocate stock_locate
    ) const noexcept;

    [[nodiscard]] bool contains(const InstrumentKey& key) const noexcept;

    [[nodiscard]] std::vector<InstrumentKey> instruments_for_symbol(
        const CanonicalSymbol& symbol
    ) const;

    [[nodiscard]] std::size_t size() const noexcept;

private:
    [[nodiscard]] static SymbolTradingStatus to_symbol_trading_status(
        itch::TradingState state
    ) noexcept;

    [[nodiscard]] static CanonicalSymbol canonical_symbol_from_stock(
        const itch::StockSymbol& stock
    ) noexcept;

    std::unordered_map<
        InstrumentKey,
        InstrumentMetadata,
        InstrumentKeyHash
    > instruments_{};

    std::unordered_map<
        CanonicalSymbol,
        std::vector<InstrumentKey>,
        CanonicalSymbolHash
    > symbol_index_{};
};

} // namespace fgep::instrument