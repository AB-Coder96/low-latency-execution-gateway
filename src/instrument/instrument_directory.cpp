#include "fgep/instrument/instrument_directory.hpp"

namespace fgep::instrument {

ErrorCode InstrumentDirectory::apply(
    venue::VenueId venue_id,
    const itch::StockDirectoryMessage& message
) {
    if (!venue::is_valid_venue_id(venue_id)) {
        return ErrorCode::invalid_argument;
    }

    if (!itch::is_valid_stock_directory_message(message)) {
        return ErrorCode::invalid_argument;
    }

    const InstrumentKey key{
        venue_id,
        message.header.stock_locate
    };

    const auto canonical_symbol = canonical_symbol_from_stock(message.stock);
    const auto existing = instruments_.find(key);

    if (existing != instruments_.end()) {
        if (existing->second.stock != message.stock) {
            return ErrorCode::invalid_state;
        }

        existing->second.market_category = message.market_category;
        existing->second.financial_status_indicator =
            message.financial_status_indicator;
        existing->second.round_lot_size = message.round_lot_size;
        existing->second.round_lots_only = message.round_lots_only;
        existing->second.issue_classification = message.issue_classification;
        existing->second.issue_sub_type = message.issue_sub_type;
        existing->second.authenticity = message.authenticity;
        existing->second.short_sale_threshold_indicator =
            message.short_sale_threshold_indicator;
        existing->second.ipo_flag = message.ipo_flag;
        existing->second.luld_reference_price_tier =
            message.luld_reference_price_tier;
        existing->second.etp_flag = message.etp_flag;
        existing->second.etp_leverage_factor = message.etp_leverage_factor;
        existing->second.inverse_indicator = message.inverse_indicator;

        return ErrorCode::ok;
    }

    InstrumentMetadata metadata{};

    metadata.key = key;
    metadata.stock = message.stock;
    metadata.market_category = message.market_category;
    metadata.financial_status_indicator =
        message.financial_status_indicator;
    metadata.round_lot_size = message.round_lot_size;
    metadata.round_lots_only = message.round_lots_only;
    metadata.issue_classification = message.issue_classification;
    metadata.issue_sub_type = message.issue_sub_type;
    metadata.authenticity = message.authenticity;
    metadata.short_sale_threshold_indicator =
        message.short_sale_threshold_indicator;
    metadata.ipo_flag = message.ipo_flag;
    metadata.luld_reference_price_tier =
        message.luld_reference_price_tier;
    metadata.etp_flag = message.etp_flag;
    metadata.etp_leverage_factor = message.etp_leverage_factor;
    metadata.inverse_indicator = message.inverse_indicator;

    instruments_.emplace(key, metadata);
    symbol_index_[canonical_symbol].push_back(key);

    return ErrorCode::ok;
}

ErrorCode InstrumentDirectory::apply(
    venue::VenueId venue_id,
    const itch::StockTradingActionMessage& message
) {
    if (!venue::is_valid_venue_id(venue_id)) {
        return ErrorCode::invalid_argument;
    }

    if (!itch::is_valid_stock_trading_action_message(message)) {
        return ErrorCode::invalid_argument;
    }

    const InstrumentKey key{
        venue_id,
        message.header.stock_locate
    };

    const auto existing = instruments_.find(key);

    if (existing == instruments_.end()) {
        return ErrorCode::not_found;
    }

    if (existing->second.stock != message.stock) {
        return ErrorCode::invalid_state;
    }

    existing->second.trading_status =
        to_symbol_trading_status(message.trading_state);

    existing->second.last_trading_action_reason = message.reason;

    return ErrorCode::ok;
}

const InstrumentMetadata* InstrumentDirectory::find(
    const InstrumentKey& key
) const noexcept {
    const auto existing = instruments_.find(key);

    if (existing == instruments_.end()) {
        return nullptr;
    }

    return &existing->second;
}

const InstrumentMetadata* InstrumentDirectory::find(
    venue::VenueId venue_id,
    itch::StockLocate stock_locate
) const noexcept {
    return find(InstrumentKey{venue_id, stock_locate});
}

bool InstrumentDirectory::contains(const InstrumentKey& key) const noexcept {
    return find(key) != nullptr;
}

std::vector<InstrumentKey> InstrumentDirectory::instruments_for_symbol(
    const CanonicalSymbol& symbol
) const {
    const auto existing = symbol_index_.find(symbol);

    if (existing == symbol_index_.end()) {
        return {};
    }

    return existing->second;
}

std::size_t InstrumentDirectory::size() const noexcept {
    return instruments_.size();
}

SymbolTradingStatus InstrumentDirectory::to_symbol_trading_status(
    itch::TradingState state
) noexcept {
    switch (state) {
        case itch::TradingState::halted:
            return SymbolTradingStatus::halted;
        case itch::TradingState::paused:
            return SymbolTradingStatus::paused;
        case itch::TradingState::quotation_only:
            return SymbolTradingStatus::quotation_only;
        case itch::TradingState::trading:
            return SymbolTradingStatus::trading;
    }

    return SymbolTradingStatus::unknown;
}

CanonicalSymbol InstrumentDirectory::canonical_symbol_from_stock(
    const itch::StockSymbol& stock
) noexcept {
    return CanonicalSymbol{stock};
}

} // namespace fgep::instrument