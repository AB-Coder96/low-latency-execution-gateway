#pragma once

#include "fgep/core/types.hpp"
#include "fgep/ouch/ouch_wire_messages.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_set>

namespace fgep::risk {

enum class RiskDecision : std::uint8_t {
    accepted,
    rejected
};

enum class RiskRejectReason : std::uint8_t {
    none,
    global_halt,
    symbol_disabled,
    cancel_only,
    max_order_quantity,
    max_order_notional,
    invalid_quantity,
    invalid_price
};

struct RiskLimits {
    ouch::Quantity max_order_quantity{100'000};
    Notional max_order_notional{1'000'000'000'000ULL};
};

struct RiskResult {
    RiskDecision decision{RiskDecision::rejected};
    RiskRejectReason reject_reason{RiskRejectReason::none};
    ouch::UserRefNum user_ref_num{};
    ouch::Quantity quantity{};
    ouch::Price4 price{};
    Notional notional{};

    [[nodiscard]] constexpr bool accepted() const noexcept {
        return decision == RiskDecision::accepted;
    }

    [[nodiscard]] constexpr bool rejected() const noexcept {
        return decision == RiskDecision::rejected;
    }
};

class RiskSupervisor {
public:
    RiskSupervisor() = default;

    explicit RiskSupervisor(RiskLimits limits) noexcept;

    [[nodiscard]] RiskResult check_enter(
        const ouch::EnterOrderMessage& message
    ) const noexcept;

    [[nodiscard]] RiskResult check_replace(
        const ouch::ReplaceOrderMessage& message,
        const ouch::Symbol& current_symbol
    ) const noexcept;

    [[nodiscard]] RiskResult check_cancel(
        const ouch::CancelOrderMessage& message
    ) const noexcept;

    void set_limits(RiskLimits limits) noexcept;
    [[nodiscard]] RiskLimits limits() const noexcept;

    void set_global_halted(bool halted) noexcept;
    [[nodiscard]] bool global_halted() const noexcept;

    void set_cancel_only(bool enabled) noexcept;
    [[nodiscard]] bool cancel_only() const noexcept;

    void set_default_symbol_enabled(bool enabled) noexcept;
    [[nodiscard]] bool default_symbol_enabled() const noexcept;

    void enable_symbol(const ouch::Symbol& symbol);
    void disable_symbol(const ouch::Symbol& symbol);

    [[nodiscard]] bool is_symbol_enabled(
        const ouch::Symbol& symbol
    ) const noexcept;

    void clear_symbol_overrides();

private:
    struct SymbolHash {
        [[nodiscard]] std::size_t operator()(
            const ouch::Symbol& symbol
        ) const noexcept;
    };

    [[nodiscard]] RiskResult reject(
        RiskRejectReason reason,
        ouch::UserRefNum user_ref_num,
        ouch::Quantity quantity,
        ouch::Price4 price,
        Notional notional = 0
    ) const noexcept;

    [[nodiscard]] RiskResult accept(
        ouch::UserRefNum user_ref_num,
        ouch::Quantity quantity,
        ouch::Price4 price,
        Notional notional
    ) const noexcept;

    [[nodiscard]] RiskResult check_common_order_risk(
        ouch::UserRefNum user_ref_num,
        ouch::Quantity quantity,
        ouch::Price4 price,
        const ouch::Symbol& symbol
    ) const noexcept;

    [[nodiscard]] bool exceeds_notional_limit(
        ouch::Quantity quantity,
        ouch::Price4 price
    ) const noexcept;

    [[nodiscard]] Notional calculate_order_notional(
        ouch::Quantity quantity,
        ouch::Price4 price
    ) const noexcept;

    RiskLimits limits_{};
    bool global_halted_{false};
    bool cancel_only_{false};
    bool default_symbol_enabled_{true};

    std::unordered_set<ouch::Symbol, SymbolHash> enabled_symbols_{};
    std::unordered_set<ouch::Symbol, SymbolHash> disabled_symbols_{};
};

} // namespace fgep::risk