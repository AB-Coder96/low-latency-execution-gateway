#include "fgep/gate/guardrail_control.hpp"

namespace fgep::gate {

GuardrailUpdate GuardrailControlPath::push_set_global_halt(bool halted) {
    auto update = make_update(GuardrailUpdateKind::set_global_halt);
    update.bool_value = halted;
    enqueue(update);
    return update;
}

GuardrailUpdate GuardrailControlPath::push_set_cancel_only(bool enabled) {
    auto update = make_update(GuardrailUpdateKind::set_cancel_only);
    update.bool_value = enabled;
    enqueue(update);
    return update;
}

GuardrailUpdate GuardrailControlPath::push_set_default_symbol_enabled(
    bool enabled
) {
    auto update = make_update(
        GuardrailUpdateKind::set_default_symbol_enabled
    );
    update.bool_value = enabled;
    enqueue(update);
    return update;
}

GuardrailUpdate GuardrailControlPath::push_set_limits(
    GuardrailLimits limits
) {
    auto update = make_update(GuardrailUpdateKind::set_limits);
    update.limits = limits;
    enqueue(update);
    return update;
}

GuardrailUpdate GuardrailControlPath::push_enable_symbol(
    const ouch::Symbol& symbol
) {
    auto update = make_update(GuardrailUpdateKind::enable_symbol);
    update.symbol = symbol;
    enqueue(update);
    return update;
}

GuardrailUpdate GuardrailControlPath::push_disable_symbol(
    const ouch::Symbol& symbol
) {
    auto update = make_update(GuardrailUpdateKind::disable_symbol);
    update.symbol = symbol;
    enqueue(update);
    return update;
}

GuardrailUpdate GuardrailControlPath::push_clear_symbol_overrides() {
    auto update = make_update(GuardrailUpdateKind::clear_symbol_overrides);
    enqueue(update);
    return update;
}

void GuardrailControlPath::enqueue(const GuardrailUpdate& update) {
    pending_.push_back(update);
    ++stats_.staged_count;

    if (update.sequence >= next_sequence_) {
        next_sequence_ = update.sequence + 1U;
    }
}

GuardrailApplyResult GuardrailControlPath::apply_next(
    GuardrailState& state
) {
    if (pending_.empty()) {
        return GuardrailApplyResult{
            .decision = GuardrailApplyDecision::no_pending,
            .sequence = 0,
            .kind = GuardrailUpdateKind::set_global_halt
        };
    }

    const auto update = pending_.front();
    pending_.erase(pending_.begin());

    return apply_update(state, update);
}

std::size_t GuardrailControlPath::apply_all(GuardrailState& state) {
    std::size_t applied_count = 0;

    while (!pending_.empty()) {
        const auto result = apply_next(state);

        if (result.applied()) {
            ++applied_count;
        }
    }

    return applied_count;
}

bool GuardrailControlPath::empty() const noexcept {
    return pending_.empty();
}

std::size_t GuardrailControlPath::pending_count() const noexcept {
    return pending_.size();
}

GuardrailUpdateSequence GuardrailControlPath::next_sequence() const noexcept {
    return next_sequence_;
}

GuardrailControlStats GuardrailControlPath::stats() const noexcept {
    return stats_;
}

void GuardrailControlPath::clear_pending() noexcept {
    pending_.clear();
}

void GuardrailControlPath::reset() noexcept {
    next_sequence_ = 1;
    stats_ = GuardrailControlStats{};
    pending_.clear();
}

GuardrailUpdate GuardrailControlPath::make_update(
    GuardrailUpdateKind kind
) noexcept {
    const auto sequence = next_sequence_;
    ++next_sequence_;

    return GuardrailUpdate{
        .sequence = sequence,
        .kind = kind,
        .bool_value = false,
        .limits = {},
        .symbol = {}
    };
}

GuardrailApplyResult GuardrailControlPath::apply_update(
    GuardrailState& state,
    const GuardrailUpdate& update
) noexcept {
    if (update.sequence <= stats_.last_applied_sequence) {
        ++stats_.dropped_stale_count;

        return GuardrailApplyResult{
            .decision = GuardrailApplyDecision::dropped_stale,
            .sequence = update.sequence,
            .kind = update.kind
        };
    }

    switch (update.kind) {
    case GuardrailUpdateKind::set_global_halt:
        state.set_global_halted(update.bool_value);
        break;

    case GuardrailUpdateKind::set_cancel_only:
        state.set_cancel_only(update.bool_value);
        break;

    case GuardrailUpdateKind::set_default_symbol_enabled:
        state.set_default_symbol_enabled(update.bool_value);
        break;

    case GuardrailUpdateKind::set_limits:
        state.set_limits(update.limits);
        break;

    case GuardrailUpdateKind::enable_symbol:
        state.enable_symbol(update.symbol);
        break;

    case GuardrailUpdateKind::disable_symbol:
        state.disable_symbol(update.symbol);
        break;

    case GuardrailUpdateKind::clear_symbol_overrides:
        state.clear_symbol_overrides();
        break;
    }

    ++stats_.applied_count;
    stats_.last_applied_sequence = update.sequence;

    return GuardrailApplyResult{
        .decision = GuardrailApplyDecision::applied,
        .sequence = update.sequence,
        .kind = update.kind
    };
}

} // namespace fgep::gate