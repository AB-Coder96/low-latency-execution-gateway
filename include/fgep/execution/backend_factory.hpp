#pragma once

#include "fgep/execution/afxdp_backend.hpp"
#include "fgep/execution/dpdk_backend.hpp"
#include "fgep/execution/execution_backend.hpp"
#include "fgep/execution/kernel_udp_backend.hpp"

#include <memory>
#include <string_view>

namespace fgep::execution {

enum class BackendTechnology {
    recording,
    kernel_udp,
    afxdp,
    dpdk
};

struct BackendFactoryConfig {
    BackendTechnology technology{BackendTechnology::kernel_udp};
    KernelUdpBackendConfig kernel_udp{};
    AfXdpBackendConfig afxdp{};
    DpdkBackendConfig dpdk{};
    std::size_t recording_max_submissions{};
};

[[nodiscard]] std::unique_ptr<ExecutionBackend> make_execution_backend(
    const BackendFactoryConfig& config
);

[[nodiscard]] BackendTechnology backend_technology_from_string(
    std::string_view text
) noexcept;

[[nodiscard]] const char* backend_technology_name(
    BackendTechnology technology
) noexcept;

[[nodiscard]] BackendTechnology default_backend_technology() noexcept;

} // namespace fgep::execution