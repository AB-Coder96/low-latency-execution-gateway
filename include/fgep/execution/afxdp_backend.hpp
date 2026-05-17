#pragma once

#include "fgep/execution/execution_backend.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace fgep::execution {

struct AfXdpBackendConfig {
    std::string interface_name{};
    std::uint32_t queue_id{};
    std::size_t umem_frame_count{4096};
    std::size_t umem_frame_size{2048};
    std::size_t tx_batch_size{64};
};

class AfXdpExecutionBackend final : public ExecutionBackend {
public:
    AfXdpExecutionBackend();

    explicit AfXdpExecutionBackend(AfXdpBackendConfig config);

    [[nodiscard]] BackendSubmitResult submit(
        const BackendSubmitRequest& request
    ) override;

    [[nodiscard]] bool configured() const noexcept;

    [[nodiscard]] bool open() const noexcept;

    [[nodiscard]] const AfXdpBackendConfig& config() const noexcept;

    [[nodiscard]] const std::string& interface_name() const noexcept;

    [[nodiscard]] std::uint32_t queue_id() const noexcept;

    void close() noexcept;

private:
    AfXdpBackendConfig config_{};
    bool configured_{false};
    bool open_{false};

    [[nodiscard]] static bool is_valid_config(
        const AfXdpBackendConfig& config
    ) noexcept;

    [[nodiscard]] BackendSubmitResult reject(
        const BackendSubmitRequest& request,
        BackendRejectReason reason
    ) const noexcept;
};

} // namespace fgep::execution