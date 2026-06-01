#include "fgep/runtime/runtime_phase.hpp"

namespace fgep::runtime {

std::string_view runtime_phase_name(RuntimePhase phase) noexcept {
    switch (phase) {
    case RuntimePhase::initialization:
        return "initialization";
    case RuntimePhase::warmup:
        return "warmup";
    case RuntimePhase::running:
        return "running";
    case RuntimePhase::stopped:
        return "stopped";
    }

    return "unknown";
}

void RuntimePhaseController::enter_initialization() noexcept {
    phase_ = RuntimePhase::initialization;
}

void RuntimePhaseController::enter_warmup() noexcept {
    phase_ = RuntimePhase::warmup;
}

void RuntimePhaseController::enter_running() noexcept {
    phase_ = RuntimePhase::running;
}

void RuntimePhaseController::enter_stopped() noexcept {
    phase_ = RuntimePhase::stopped;
}

RuntimePhase RuntimePhaseController::phase() const noexcept {
    return phase_;
}

bool RuntimePhaseController::runtime_active() const noexcept {
    return phase_ == RuntimePhase::running;
}

} // namespace fgep::runtime