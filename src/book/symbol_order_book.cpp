#include "fgep/book/symbol_order_book.hpp"

#include <variant>

namespace fgep::book {

SymbolOrderBook::SymbolOrderBook(
    instrument::InstrumentKey key,
    itch::StockSymbol stock
)
    : key_{key},
      stock_{stock} {
}

const instrument::InstrumentKey& SymbolOrderBook::key() const noexcept {
    return key_;
}

const itch::StockSymbol& SymbolOrderBook::stock() const noexcept {
    return stock_;
}

ErrorCode SymbolOrderBook::apply(const itch::Message& message) {
    return std::visit(
        [this](const auto& concrete_message) -> ErrorCode {
            return apply(concrete_message);
        },
        message
    );
}

ErrorCode SymbolOrderBook::apply(const itch::SystemEventMessage&) {
    return ErrorCode::ok;
}

ErrorCode SymbolOrderBook::apply(const itch::StockDirectoryMessage&) {
    return ErrorCode::ok;
}

ErrorCode SymbolOrderBook::apply(const itch::StockTradingActionMessage&) {
    return ErrorCode::ok;
}

ErrorCode SymbolOrderBook::apply(
    const itch::AddOrderNoMpidMessage& message
) {
    auto error = require_matching_header(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    return add_order(
        message.order_reference_number,
        message.side,
        message.shares,
        message.stock,
        message.price
    );
}

ErrorCode SymbolOrderBook::apply(
    const itch::AddOrderWithMpidMessage& message
) {
    auto error = require_matching_header(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    return add_order(
        message.order_reference_number,
        message.side,
        message.shares,
        message.stock,
        message.price
    );
}

ErrorCode SymbolOrderBook::apply(
    const itch::OrderExecutedMessage& message
) {
    auto error = require_matching_header(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    return reduce_order(
        message.order_reference_number,
        message.executed_shares
    );
}

ErrorCode SymbolOrderBook::apply(
    const itch::OrderExecutedWithPriceMessage& message
) {
    auto error = require_matching_header(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    return reduce_order(
        message.order_reference_number,
        message.executed_shares
    );
}

ErrorCode SymbolOrderBook::apply(const itch::OrderCancelMessage& message) {
    auto error = require_matching_header(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    return reduce_order(
        message.order_reference_number,
        message.cancelled_shares
    );
}

ErrorCode SymbolOrderBook::apply(const itch::OrderDeleteMessage& message) {
    auto error = require_matching_header(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    return delete_order(message.order_reference_number);
}

ErrorCode SymbolOrderBook::apply(const itch::OrderReplaceMessage& message) {
    auto error = require_matching_header(message.header);

    if (error != ErrorCode::ok) {
        return error;
    }

    const auto existing = orders_.find(
        message.original_order_reference_number
    );

    if (existing == orders_.end()) {
        return ErrorCode::not_found;
    }

    if (!itch::is_valid_shares(message.shares)
        || !itch::is_valid_price4(message.price)) {
        return ErrorCode::invalid_argument;
    }

    if (orders_.contains(message.new_order_reference_number)) {
        return ErrorCode::duplicate;
    }

    const RestingOrder old_order = existing->second;

    remove_from_level(old_order, old_order.shares);
    orders_.erase(existing);

    return add_order(
        message.new_order_reference_number,
        old_order.side,
        message.shares,
        stock_,
        message.price
    );
}

ErrorCode SymbolOrderBook::apply(const itch::TradeNonCrossMessage&) {
    return ErrorCode::ok;
}

ErrorCode SymbolOrderBook::apply(const itch::CrossTradeMessage&) {
    return ErrorCode::ok;
}

ErrorCode SymbolOrderBook::apply(const itch::BrokenTradeMessage&) {
    return ErrorCode::ok;
}

std::optional<Level> SymbolOrderBook::best_bid() const noexcept {
    if (bids_.empty()) {
        return std::nullopt;
    }

    const auto& [price, state] = *bids_.begin();
    return make_level(price, state);
}

std::optional<Level> SymbolOrderBook::best_ask() const noexcept {
    if (asks_.empty()) {
        return std::nullopt;
    }

    const auto& [price, state] = *asks_.begin();
    return make_level(price, state);
}

bool SymbolOrderBook::contains_order(
    itch::OrderReferenceNumber order_reference_number
) const noexcept {
    return orders_.contains(order_reference_number);
}

std::size_t SymbolOrderBook::order_count() const noexcept {
    return orders_.size();
}

bool SymbolOrderBook::empty() const noexcept {
    return orders_.empty();
}

ErrorCode SymbolOrderBook::require_matching_header(
    const itch::Header& header
) const noexcept {
    if (!instrument::is_valid_instrument_key(key_)) {
        return ErrorCode::invalid_state;
    }

    if (header.stock_locate != key_.stock_locate) {
        return ErrorCode::invalid_argument;
    }

    if (!itch::is_valid_timestamp_ns(header.timestamp_ns)) {
        return ErrorCode::invalid_argument;
    }

    return ErrorCode::ok;
}

ErrorCode SymbolOrderBook::require_matching_stock(
    const itch::StockSymbol& stock
) const noexcept {
    if (stock != stock_) {
        return ErrorCode::invalid_argument;
    }

    return ErrorCode::ok;
}

ErrorCode SymbolOrderBook::add_order(
    itch::OrderReferenceNumber order_reference_number,
    itch::Side side,
    itch::Shares shares,
    const itch::StockSymbol& stock,
    itch::Price4 price
) {
    if (!itch::is_valid_order_reference_number(order_reference_number)
        || !itch::is_valid_shares(shares)
        || !itch::is_valid_price4(price)) {
        return ErrorCode::invalid_argument;
    }

    auto error = require_matching_stock(stock);

    if (error != ErrorCode::ok) {
        return error;
    }

    if (orders_.contains(order_reference_number)) {
        return ErrorCode::duplicate;
    }

    RestingOrder order{
        order_reference_number,
        side,
        shares,
        price
    };

    add_to_level(order);
    orders_.emplace(order_reference_number, order);

    return ErrorCode::ok;
}

ErrorCode SymbolOrderBook::reduce_order(
    itch::OrderReferenceNumber order_reference_number,
    itch::Shares shares_to_reduce
) {
    if (!itch::is_valid_shares(shares_to_reduce)) {
        return ErrorCode::invalid_argument;
    }

    const auto existing = orders_.find(order_reference_number);

    if (existing == orders_.end()) {
        return ErrorCode::not_found;
    }

    if (shares_to_reduce > existing->second.shares) {
        return ErrorCode::invalid_state;
    }

    remove_from_level(existing->second, shares_to_reduce);
    existing->second.shares -= shares_to_reduce;

    if (existing->second.shares == 0) {
        orders_.erase(existing);
    }

    return ErrorCode::ok;
}

ErrorCode SymbolOrderBook::delete_order(
    itch::OrderReferenceNumber order_reference_number
) {
    const auto existing = orders_.find(order_reference_number);

    if (existing == orders_.end()) {
        return ErrorCode::not_found;
    }

    remove_from_level(existing->second, existing->second.shares);
    orders_.erase(existing);

    return ErrorCode::ok;
}

void SymbolOrderBook::add_to_level(const RestingOrder& order) {
    if (order.side == itch::Side::buy) {
        auto& state = bids_[order.price];
        state.quantity += static_cast<Quantity>(order.shares);
        ++state.order_count;
        return;
    }

    auto& state = asks_[order.price];
    state.quantity += static_cast<Quantity>(order.shares);
    ++state.order_count;
}

void SymbolOrderBook::remove_from_level(
    const RestingOrder& order,
    itch::Shares shares
) {
    auto remove_from_state = [&order, shares](LevelState& state) {
        const auto quantity_to_remove = static_cast<Quantity>(shares);

        if (quantity_to_remove >= state.quantity) {
            state.quantity = 0;
        } else {
            state.quantity -= quantity_to_remove;
        }

        if (shares == order.shares && state.order_count > 0) {
            --state.order_count;
        }
    };

    if (order.side == itch::Side::buy) {
        const auto existing_level = bids_.find(order.price);

        if (existing_level == bids_.end()) {
            return;
        }

        remove_from_state(existing_level->second);

        if (is_empty(make_level(order.price, existing_level->second))) {
            bids_.erase(existing_level);
        }

        return;
    }

    const auto existing_level = asks_.find(order.price);

    if (existing_level == asks_.end()) {
        return;
    }

    remove_from_state(existing_level->second);

    if (is_empty(make_level(order.price, existing_level->second))) {
        asks_.erase(existing_level);
    }
}

Level SymbolOrderBook::make_level(
    itch::Price4 price,
    const LevelState& state
) noexcept {
    return Level{
        static_cast<Price>(price),
        state.quantity,
        state.order_count
    };
}

} // namespace fgep::book