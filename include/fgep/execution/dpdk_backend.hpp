#pragma once

#include "fgep/execution/execution_backend.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fgep::execution {

struct DpdkMacAddress {
    std::array<std::uint8_t, 6> bytes{};
};

struct DpdkBackendConfig {
    std::vector<std::string> eal_args{"fgep-dpdk", "-l", "0-1", "-n", "4"};
    std::uint16_t port_id{};
    std::uint16_t queue_id{};
    std::uint16_t tx_desc_count{1024};
    std::uint16_t burst_size{32};
    std::size_t mbuf_pool_size{8192};
    std::uint16_t mbuf_cache_size{250};
    DpdkMacAddress destination_mac{{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
    DpdkMacAddress source_mac{{0x02, 0x00, 0x00, 0x00, 0x00, 0x01}};
    std::uint16_t ether_type{0x88B5};
};

class DpdkExecutionBackend final : public ExecutionBackend {
public:
    DpdkExecutionBackend();

    explicit DpdkExecutionBackend(DpdkBackendConfig config);

    ~DpdkExecutionBackend() override;

    DpdkExecutionBackend(const DpdkExecutionBackend&) = delete;
    DpdkExecutionBackend& operator=(const DpdkExecutionBackend&) = delete;

    DpdkExecutionBackend(DpdkExecutionBackend&& other) noexcept;
    DpdkExecutionBackend& operator=(DpdkExecutionBackend&& other) noexcept;

    [[nodiscard]] BackendSubmitResult submit(
        const BackendSubmitRequest& request
    ) override;

    [[nodiscard]] bool configured() const noexcept;
    [[nodiscard]] bool open() const noexcept;
    [[nodiscard]] bool dpdk_available() const noexcept;

    [[nodiscard]] const DpdkBackendConfig& config() const noexcept;

    [[nodiscard]] std::uint16_t port_id() const noexcept;
    [[nodiscard]] std::uint16_t queue_id() const noexcept;
    [[nodiscard]] std::uint16_t burst_size() const noexcept;
    [[nodiscard]] std::size_t mbuf_pool_size() const noexcept;

    void close() noexcept;

private:
    DpdkBackendConfig config_{};
    bool configured_{false};
    bool open_{false};

#if FGEP_HAVE_DPDK
    void* mbuf_pool_{nullptr};
#endif

    [[nodiscard]] static bool is_valid_config(
        const DpdkBackendConfig& config
    ) noexcept;

    void initialize() noexcept;
    void cleanup() noexcept;

    [[nodiscard]] BackendSubmitResult reject(
        const BackendSubmitRequest& request,
        BackendRejectReason reason
    ) const noexcept;
};

} // namespace fgep::execution