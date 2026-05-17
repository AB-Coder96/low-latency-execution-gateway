#include "fgep/execution/afxdp_backend.hpp"

#include <utility>

#ifndef FGEP_ENABLE_AFXDP
#define FGEP_ENABLE_AFXDP 0
#endif

namespace fgep::execution {

AfXdpExecutionBackend::AfXdpExecutionBackend()
    : AfXdpExecutionBackend{AfXdpBackendConfig{}} {
}

AfXdpExecutionBackend::AfXdpExecutionBackend(
    AfXdpBackendConfig config
)
    : config_{std::move(config)},
      configured_{is_valid_config(config_)},
      open_{false} {
}

BackendSubmitResult AfXdpExecutionBackend::submit(
    const BackendSubmitRequest& request
) {
    if (request.payload.empty()) {
        return reject(request, BackendRejectReason::empty_payload);
    }

    if (!configured_) {
        return reject(request, BackendRejectReason::invalid_endpoint);
    }

#if FGEP_ENABLE_AFXDP
    // Real AF_XDP UMEM/ring submission will be added here later.
    return reject(request, BackendRejectReason::unsupported_backend);
#else
    return reject(request, BackendRejectReason::unsupported_backend);
#endif
}

bool AfXdpExecutionBackend::configured() const noexcept {
    return configured_;
}

bool AfXdpExecutionBackend::open() const noexcept {
    return open_;
}

const AfXdpBackendConfig& AfXdpExecutionBackend::config() const noexcept {
    return config_;
}

const std::string& AfXdpExecutionBackend::interface_name() const noexcept {
    return config_.interface_name;
}

std::uint32_t AfXdpExecutionBackend::queue_id() const noexcept {
    return config_.queue_id;
}

void AfXdpExecutionBackend::close() noexcept {
    open_ = false;
}

bool AfXdpExecutionBackend::is_valid_config(
    const AfXdpBackendConfig& config
) noexcept {
    return !config.interface_name.empty()
        && config.umem_frame_count != 0
        && config.umem_frame_size >= 2048U
        && config.tx_batch_size != 0;
}

BackendSubmitResult AfXdpExecutionBackend::reject(
    const BackendSubmitRequest& request,
    BackendRejectReason reason
) const noexcept {
    return make_backend_reject(request, reason);
}

} // namespace fgep::execution