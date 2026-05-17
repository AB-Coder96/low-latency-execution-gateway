#pragma once

#include "fgep/core/types.hpp"
#include "fgep/ouch/ouch_wire_messages.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_set>

namespace fgep::gate {

enum class GuardrailOrderKind : std::uint8_t {
    enter,
    replace,
    cancel
};

enum class GuardrailDecision : std::uint8_t {
    allowed,
    rejected
};

enum class GuardrailRejectReason : std::uint8_t {
    none,
    global_halt,
    cancel_only,
    symbol_missing,
    symbol_disabled,
    invalid_quantity,
    invalid_price,
    max_order_quantity,
    max_order_notional
};

struct GuardrailLimits {
    ouch::Quantity max_order_quantity{100'000};
    Notional max_order_notional{1'000'000'000'000ULL};
};

struct GuardrailOrder {
    GuardrailOrderKind kind{GuardrailOrderKind::enter};
    ouch::UserRefNum user_ref_num{};
    ouch::Symbol symbol{};
    bool has_symbol{false};
    ouch::Quantity quantity{};
    ouch::Price4 price{};
};

struct GuardrailResult {
    GuardrailDecision decision{GuardrailDecision::rejected};
    GuardrailRejectReason reject_reason{GuardrailRejectReason::none};
    GuardrailOrderKind kind{GuardrailOrderKind::enter};
    ouch::UserRefNum user_ref_num{};
    ouch::Quantity quantity{};
    ouch::Price4 price{};
    Notional notional{};

    [[nodiscard]] constexpr bool allowed() const noexcept {
        return decision == GuardrailDecision::allowed;
    }

    [[nodiscard]] constexpr bool rejected() const noexcept {
        return decision == GuardrailDecision::rejected;
    }
};

class GuardrailState {
public:
    GuardrailState() = default;

    explicit GuardrailState(GuardrailLimits limits) noexcept;

    [[nodiscard]] GuardrailResult check(
        const GuardrailOrder& order
    ) const noexcept;

    void set_limits(GuardrailLimits limits) noexcept;
    [[nodiscard]] GuardrailLimits limits() const noexcept;

    void set_global_halted(bool halted) noexcept;
    [[nodiscard]] bool global_halted() const noexcept;

    void set_cancel_only(bool enabled) noexcept;
    [[nodiscard]] bool cancel_only() const noexcept;

    void set_default_symbol_enabled(bool enabled) noexcept;
    [[nodiscard]] bool default_symbol_enabled() const noexcept;

    void enable_symbol(const ouch::Symbol& symbol);
    void disable_symbol(const ouch::Symbol& symbol);
    void clear_symbol_overrides();

    [[nodiscard]] bool is_symbol_enabled(
        const ouch::Symbol& symbol
    ) const noexcept;

private:
    struct SymbolHash {
        [[nodiscard]] std::size_t operator()(
            const ouch::Symbol& symbol
        ) const noexcept;
    };

    [[nodiscard]] GuardrailResult allow(
        const GuardrailOrder& order,
        Notional notional
    ) const noexcept;

    [[nodiscard]] GuardrailResult reject(
        const GuardrailOrder& order,
        GuardrailRejectReason reason,
        Notional notional = 0
    ) const noexcept;

    [[nodiscard]] bool requires_symbol(
        GuardrailOrderKind kind
    ) const noexcept;

    [[nodiscard]] bool is_cancel(
        GuardrailOrderKind kind
    ) const noexcept;

    [[nodiscard]] Notional calculate_notional(
        ouch::Quantity quantity,
        ouch::Price4 price
    ) const noexcept;

    [[nodiscard]] bool exceeds_notional_limit(
        ouch::Quantity quantity,
        ouch::Price4 price
    ) const noexcept;

    GuardrailLimits limits_{};
    bool global_halted_{false};
    bool cancel_only_{false};
    bool default_symbol_enabled_{true};

    std::unordered_set<ouch::Symbol, SymbolHash> enabled_symbols_{};
    std::unordered_set<ouch::Symbol, SymbolHash> disabled_symbols_{};
};

[[nodiscard]] GuardrailOrder make_enter_guardrail_order(
    const ouch::EnterOrderMessage& message
) noexcept;

[[nodiscard]] GuardrailOrder make_replace_guardrail_order(
    const ouch::ReplaceOrderMessage& message,
    const ouch::Symbol& current_symbol
) noexcept;

[[nodiscard]] GuardrailOrder make_cancel_guardrail_order(
    const ouch::CancelOrderMessage& message
) noexcept;

} // namespace fgep::gate