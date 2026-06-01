#include "fgep/runtime/runtime_phase.hpp"

#include <cassert>
#include <type_traits>

int main() {
    using namespace fgep::runtime;

    static_assert(std::is_nothrow_default_constructible_v<RuntimePhaseController>);

    {
        assert(runtime_phase_name(RuntimePhase::initialization) == "initialization");
        assert(runtime_phase_name(RuntimePhase::warmup) == "warmup");
        assert(runtime_phase_name(RuntimePhase::running) == "running");
        assert(runtime_phase_name(RuntimePhase::stopped) == "stopped");
    }

    {
        RuntimePhaseController controller{};

        assert(controller.phase() == RuntimePhase::initialization);
        assert(!controller.runtime_active());

        controller.enter_warmup();

        assert(controller.phase() == RuntimePhase::warmup);
        assert(!controller.runtime_active());

        controller.enter_running();

        assert(controller.phase() == RuntimePhase::running);
        assert(controller.runtime_active());

        controller.enter_stopped();

        assert(controller.phase() == RuntimePhase::stopped);
        assert(!controller.runtime_active());
    }

    {
        RuntimePhaseController controller{};

        controller.enter_running();
        assert(controller.phase() == RuntimePhase::running);
        assert(controller.runtime_active());

        controller.enter_initialization();
        assert(controller.phase() == RuntimePhase::initialization);
        assert(!controller.runtime_active());
    }

    return 0;
}