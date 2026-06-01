#pragma once

#include <string_view>

namespace fgep::runtime {

enum class RuntimePhase {
    initialization,
    warmup,
    running,
    stopped
};

[[nodiscard]] std::string_view runtime_phase_name(
    RuntimePhase phase
) noexcept;

class RuntimePhaseController {
public:
    RuntimePhaseController() = default;

    void enter_initialization() noexcept;
    void enter_warmup() noexcept;
    void enter_running() noexcept;
    void enter_stopped() noexcept;

    [[nodiscard]] RuntimePhase phase() const noexcept;
    [[nodiscard]] bool runtime_active() const noexcept;

private:
    RuntimePhase phase_{RuntimePhase::initialization};
};

} // namespace fgep::runtime