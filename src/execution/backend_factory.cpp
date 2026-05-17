#include "fgep/execution/backend_factory.hpp"

namespace fgep::execution {

std::unique_ptr<ExecutionBackend> make_execution_backend(
    const BackendFactoryConfig& config
) {
    switch (config.technology) {
    case BackendTechnology::recording:
        return std::make_unique<RecordingExecutionBackend>(
            config.recording_max_submissions
        );

    case BackendTechnology::kernel_udp:
        return std::make_unique<KernelUdpExecutionBackend>(
            config.kernel_udp
        );

    case BackendTechnology::afxdp:
        return std::make_unique<AfXdpExecutionBackend>(
            config.afxdp
        );

    case BackendTechnology::dpdk:
        return std::make_unique<DpdkExecutionBackend>(
            config.dpdk
        );
    }

    return std::make_unique<RecordingExecutionBackend>();
}

BackendTechnology backend_technology_from_string(
    std::string_view text
) noexcept {
    if (text == "recording") {
        return BackendTechnology::recording;
    }

    if (text == "kernel_udp") {
        return BackendTechnology::kernel_udp;
    }

    if (text == "afxdp") {
        return BackendTechnology::afxdp;
    }

    if (text == "dpdk") {
        return BackendTechnology::dpdk;
    }

    return BackendTechnology::kernel_udp;
}

const char* backend_technology_name(
    BackendTechnology technology
) noexcept {
    switch (technology) {
    case BackendTechnology::recording:
        return "recording";
    case BackendTechnology::kernel_udp:
        return "kernel_udp";
    case BackendTechnology::afxdp:
        return "afxdp";
    case BackendTechnology::dpdk:
        return "dpdk";
    }

    return "kernel_udp";
}

BackendTechnology default_backend_technology() noexcept {
    return backend_technology_from_string(FGEP_DEFAULT_EXEC_BACKEND);
}

} // namespace fgep::execution