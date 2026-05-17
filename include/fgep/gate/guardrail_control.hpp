#pragma once

#include "fgep/gate/guardrail_state.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fgep::gate {

using GuardrailUpdateSequence = std::uint64_t;

enum class GuardrailUpdateKind : std::uint8_t {
    set_global_halt,
    set_cancel_only,
    set_default_symbol_enabled,
    set_limits,
    enable_symbol,
    disable_symbol,
    clear_symbol_overrides
};

enum class GuardrailApplyDecision : std::uint8_t {
    applied,
    dropped_stale,
    no_pending
};

struct GuardrailUpdate {
    GuardrailUpdateSequence sequence{};
    GuardrailUpdateKind kind{GuardrailUpdateKind::set_global_halt};
    bool bool_value{};
    GuardrailLimits limits{};
    ouch::Symbol symbol{};
};

struct GuardrailApplyResult {
    GuardrailApplyDecision decision{GuardrailApplyDecision::no_pending};
    GuardrailUpdateSequence sequence{};
    GuardrailUpdateKind kind{GuardrailUpdateKind::set_global_halt};

    [[nodiscard]] constexpr bool applied() const noexcept {
        return decision == GuardrailApplyDecision::applied;
    }

    [[nodiscard]] constexpr bool dropped() const noexcept {
        return decision == GuardrailApplyDecision::dropped_stale;
    }
};

struct GuardrailControlStats {
    std::size_t staged_count{};
    std::size_t applied_count{};
    std::size_t dropped_stale_count{};
    GuardrailUpdateSequence last_applied_sequence{};
};

class GuardrailControlPath {
public:
    [[nodiscard]] GuardrailUpdate push_set_global_halt(bool halted);
    [[nodiscard]] GuardrailUpdate push_set_cancel_only(bool enabled);

    [[nodiscard]] GuardrailUpdate push_set_default_symbol_enabled(
        bool enabled
    );

    [[nodiscard]] GuardrailUpdate push_set_limits(GuardrailLimits limits);

    [[nodiscard]] GuardrailUpdate push_enable_symbol(
        const ouch::Symbol& symbol
    );

    [[nodiscard]] GuardrailUpdate push_disable_symbol(
        const ouch::Symbol& symbol
    );

    [[nodiscard]] GuardrailUpdate push_clear_symbol_overrides();

    void enqueue(const GuardrailUpdate& update);

    [[nodiscard]] GuardrailApplyResult apply_next(GuardrailState& state);

    [[nodiscard]] std::size_t apply_all(GuardrailState& state);

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t pending_count() const noexcept;
    [[nodiscard]] GuardrailUpdateSequence next_sequence() const noexcept;
    [[nodiscard]] GuardrailControlStats stats() const noexcept;

    void clear_pending() noexcept;
    void reset() noexcept;

private:
    GuardrailUpdateSequence next_sequence_{1};
    GuardrailControlStats stats_{};
    std::vector<GuardrailUpdate> pending_{};

    [[nodiscard]] GuardrailUpdate make_update(
        GuardrailUpdateKind kind
    ) noexcept;

    [[nodiscard]] GuardrailApplyResult apply_update(
        GuardrailState& state,
        const GuardrailUpdate& update
    ) noexcept;
};

} // namespace fgep::gate