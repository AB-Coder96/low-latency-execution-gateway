#pragma once

#include "fgep/execution/execution_backend.hpp"

#include <cstdint>
#include <string>

namespace fgep::execution {

struct KernelUdpBackendConfig {
    std::string destination_ipv4{"127.0.0.1"};
    std::uint16_t destination_port{9000};
};

class KernelUdpExecutionBackend final : public ExecutionBackend {
public:
    KernelUdpExecutionBackend();

    explicit KernelUdpExecutionBackend(KernelUdpBackendConfig config);

    ~KernelUdpExecutionBackend() override;

    KernelUdpExecutionBackend(const KernelUdpExecutionBackend&) = delete;
    KernelUdpExecutionBackend& operator=(const KernelUdpExecutionBackend&) =
        delete;

    KernelUdpExecutionBackend(KernelUdpExecutionBackend&& other) noexcept;
    KernelUdpExecutionBackend& operator=(
        KernelUdpExecutionBackend&& other
    ) noexcept;

    [[nodiscard]] BackendSubmitResult submit(
        const BackendSubmitRequest& request
    ) override;

    [[nodiscard]] bool open() const noexcept;

    [[nodiscard]] bool endpoint_valid() const noexcept;

    [[nodiscard]] int native_socket() const noexcept;

    [[nodiscard]] const std::string& destination_ipv4() const noexcept;

    [[nodiscard]] std::uint16_t destination_port() const noexcept;

    void close() noexcept;

private:
    KernelUdpBackendConfig config_{};
    int socket_fd_{-1};
    bool endpoint_valid_{false};
    bool manually_closed_{false};

    void open_socket() noexcept;
    void close_socket() noexcept;

    [[nodiscard]] BackendSubmitResult reject(
        const BackendSubmitRequest& request,
        BackendRejectReason reason
    ) const noexcept;
};

} // namespace fgep::execution