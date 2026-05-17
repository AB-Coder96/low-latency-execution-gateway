#include "fgep/execution/dpdk_backend.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#if FGEP_HAVE_DPDK
#include <rte_byteorder.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#endif

namespace fgep::execution {
namespace {

#if FGEP_HAVE_DPDK

[[nodiscard]] std::vector<char*> make_argv(
    std::vector<std::string>& args
) {
    std::vector<char*> argv{};
    argv.reserve(args.size());

    for (auto& arg : args) {
        argv.push_back(arg.data());
    }

    return argv;
}

[[nodiscard]] bool ensure_eal_initialized(
    std::vector<std::string> eal_args
) noexcept {
    static bool initialized = false;

    if (initialized) {
        return true;
    }

    auto argv = make_argv(eal_args);
    const auto argc = static_cast<int>(argv.size());

    const auto result = rte_eal_init(argc, argv.data());

    if (result < 0) {
        return false;
    }

    initialized = true;
    return true;
}

[[nodiscard]] rte_ether_addr make_rte_mac(
    const DpdkMacAddress& address
) noexcept {
    rte_ether_addr value{};

    for (std::size_t index = 0; index < address.bytes.size(); ++index) {
        value.addr_bytes[index] = address.bytes[index];
    }

    return value;
}

void write_ethernet_header(
    rte_mbuf* mbuf,
    const DpdkBackendConfig& config
) noexcept {
    auto* header = rte_pktmbuf_mtod(mbuf, rte_ether_hdr*);

    header->dst_addr = make_rte_mac(config.destination_mac);
    header->src_addr = make_rte_mac(config.source_mac);
    header->ether_type = rte_cpu_to_be_16(config.ether_type);
}

[[nodiscard]] bool configure_port(
    const DpdkBackendConfig& config,
    rte_mempool* pool
) noexcept {
    rte_eth_conf port_conf{};
    port_conf.txmode.mq_mode = RTE_ETH_MQ_TX_NONE;

    const auto configure_result = rte_eth_dev_configure(
        config.port_id,
        0,
        1,
        &port_conf
    );

    if (configure_result < 0) {
        return false;
    }

    const auto setup_result = rte_eth_tx_queue_setup(
        config.port_id,
        config.queue_id,
        config.tx_desc_count,
        rte_eth_dev_socket_id(config.port_id),
        nullptr
    );

    if (setup_result < 0) {
        return false;
    }

    static_cast<void>(pool);

    const auto start_result = rte_eth_dev_start(config.port_id);

    if (start_result < 0) {
        return false;
    }

    return true;
}

#endif

[[nodiscard]] std::size_t ethernet_frame_size(
    std::size_t payload_size
) noexcept {
    return 14U + payload_size;
}

} // namespace

DpdkExecutionBackend::DpdkExecutionBackend()
    : DpdkExecutionBackend{DpdkBackendConfig{}} {
}

DpdkExecutionBackend::DpdkExecutionBackend(DpdkBackendConfig config)
    : config_{std::move(config)},
      configured_{is_valid_config(config_)},
      open_{false} {
    initialize();
}

DpdkExecutionBackend::~DpdkExecutionBackend() {
    cleanup();
}

DpdkExecutionBackend::DpdkExecutionBackend(
    DpdkExecutionBackend&& other
) noexcept
    : config_{std::move(other.config_)},
      configured_{other.configured_},
      open_{other.open_}
#if FGEP_HAVE_DPDK
      ,
      mbuf_pool_{other.mbuf_pool_}
#endif
{
    other.configured_ = false;
    other.open_ = false;
#if FGEP_HAVE_DPDK
    other.mbuf_pool_ = nullptr;
#endif
}

DpdkExecutionBackend& DpdkExecutionBackend::operator=(
    DpdkExecutionBackend&& other
) noexcept {
    if (this == &other) {
        return *this;
    }

    cleanup();

    config_ = std::move(other.config_);
    configured_ = other.configured_;
    open_ = other.open_;

#if FGEP_HAVE_DPDK
    mbuf_pool_ = other.mbuf_pool_;
    other.mbuf_pool_ = nullptr;
#endif

    other.configured_ = false;
    other.open_ = false;

    return *this;
}

BackendSubmitResult DpdkExecutionBackend::submit(
    const BackendSubmitRequest& request
) {
    if (request.payload.empty()) {
        return reject(request, BackendRejectReason::empty_payload);
    }

    if (!configured_) {
        return reject(request, BackendRejectReason::invalid_endpoint);
    }

#if !FGEP_HAVE_DPDK
    return reject(request, BackendRejectReason::unsupported_backend);
#else
    if (!open_ || mbuf_pool_ == nullptr) {
        return reject(request, BackendRejectReason::backend_closed);
    }

    auto* pool = static_cast<rte_mempool*>(mbuf_pool_);
    auto* mbuf = rte_pktmbuf_alloc(pool);

    if (mbuf == nullptr) {
        return reject(request, BackendRejectReason::send_failed);
    }

    const auto frame_size = ethernet_frame_size(request.payload.size());
    auto* frame = static_cast<std::byte*>(
        rte_pktmbuf_append(mbuf, static_cast<std::uint16_t>(frame_size))
    );

    if (frame == nullptr) {
        rte_pktmbuf_free(mbuf);
        return reject(request, BackendRejectReason::send_failed);
    }

    write_ethernet_header(mbuf, config_);

    auto* payload = frame + 14U;

    std::copy(
        request.payload.begin(),
        request.payload.end(),
        payload
    );

    rte_mbuf* burst[1] = {mbuf};

    const auto sent = rte_eth_tx_burst(
        config_.port_id,
        config_.queue_id,
        burst,
        1
    );

    if (sent != 1) {
        rte_pktmbuf_free(mbuf);
        return reject(request, BackendRejectReason::send_failed);
    }

    return make_backend_accept(request);
#endif
}

bool DpdkExecutionBackend::configured() const noexcept {
    return configured_;
}

bool DpdkExecutionBackend::open() const noexcept {
    return open_;
}

bool DpdkExecutionBackend::dpdk_available() const noexcept {
#if FGEP_HAVE_DPDK
    return true;
#else
    return false;
#endif
}

const DpdkBackendConfig& DpdkExecutionBackend::config() const noexcept {
    return config_;
}

std::uint16_t DpdkExecutionBackend::port_id() const noexcept {
    return config_.port_id;
}

std::uint16_t DpdkExecutionBackend::queue_id() const noexcept {
    return config_.queue_id;
}

std::uint16_t DpdkExecutionBackend::burst_size() const noexcept {
    return config_.burst_size;
}

std::size_t DpdkExecutionBackend::mbuf_pool_size() const noexcept {
    return config_.mbuf_pool_size;
}

void DpdkExecutionBackend::close() noexcept {
    cleanup();
}

bool DpdkExecutionBackend::is_valid_config(
    const DpdkBackendConfig& config
) noexcept {
    return config.burst_size != 0
        && config.mbuf_pool_size != 0
        && config.tx_desc_count != 0
        && config.eal_args.size() >= 1;
}

void DpdkExecutionBackend::initialize() noexcept {
    if (!configured_) {
        return;
    }

#if !FGEP_HAVE_DPDK
    open_ = false;
#else
    if (!ensure_eal_initialized(config_.eal_args)) {
        open_ = false;
        return;
    }

    const auto available_ports = rte_eth_dev_count_avail();

    if (config_.port_id >= available_ports) {
        open_ = false;
        return;
    }

    auto* pool = rte_pktmbuf_pool_create(
        "fgep_dpdk_mbuf_pool",
        static_cast<unsigned int>(config_.mbuf_pool_size),
        config_.mbuf_cache_size,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id()
    );

    if (pool == nullptr) {
        open_ = false;
        return;
    }

    mbuf_pool_ = pool;

    if (!configure_port(config_, pool)) {
        cleanup();
        return;
    }

    open_ = true;
#endif
}

void DpdkExecutionBackend::cleanup() noexcept {
#if FGEP_HAVE_DPDK
    if (open_) {
        rte_eth_dev_stop(config_.port_id);
    }

    if (mbuf_pool_ != nullptr) {
        rte_mempool_free(static_cast<rte_mempool*>(mbuf_pool_));
        mbuf_pool_ = nullptr;
    }
#endif

    open_ = false;
}

BackendSubmitResult DpdkExecutionBackend::reject(
    const BackendSubmitRequest& request,
    BackendRejectReason reason
) const noexcept {
    return make_backend_reject(request, reason);
}

} // namespace fgep::execution