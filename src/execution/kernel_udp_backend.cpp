#include "fgep/execution/kernel_udp_backend.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstddef>
#include <utility>

namespace fgep::execution {
namespace {

[[nodiscard]] bool is_valid_ipv4(
    const std::string& address
) noexcept {
    in_addr parsed{};

    return ::inet_pton(AF_INET, address.c_str(), &parsed) == 1;
}

[[nodiscard]] bool is_valid_endpoint(
    const KernelUdpBackendConfig& config
) noexcept {
    return config.destination_port != 0
        && is_valid_ipv4(config.destination_ipv4);
}

[[nodiscard]] sockaddr_in make_destination(
    const KernelUdpBackendConfig& config
) noexcept {
    sockaddr_in destination{};
    destination.sin_family = AF_INET;
    destination.sin_port = htons(config.destination_port);

    static_cast<void>(
        ::inet_pton(
            AF_INET,
            config.destination_ipv4.c_str(),
            &destination.sin_addr
        )
    );

    return destination;
}

} // namespace

KernelUdpExecutionBackend::KernelUdpExecutionBackend()
    : KernelUdpExecutionBackend{KernelUdpBackendConfig{}} {
}

KernelUdpExecutionBackend::KernelUdpExecutionBackend(
    KernelUdpBackendConfig config
)
    : config_{std::move(config)},
      endpoint_valid_{is_valid_endpoint(config_)} {
    open_socket();
}

KernelUdpExecutionBackend::~KernelUdpExecutionBackend() {
    close_socket();
}

KernelUdpExecutionBackend::KernelUdpExecutionBackend(
    KernelUdpExecutionBackend&& other
) noexcept
    : config_{std::move(other.config_)},
      socket_fd_{other.socket_fd_},
      endpoint_valid_{other.endpoint_valid_},
      manually_closed_{other.manually_closed_} {
    other.socket_fd_ = -1;
    other.endpoint_valid_ = false;
    other.manually_closed_ = true;
}

KernelUdpExecutionBackend& KernelUdpExecutionBackend::operator=(
    KernelUdpExecutionBackend&& other
) noexcept {
    if (this == &other) {
        return *this;
    }

    close_socket();

    config_ = std::move(other.config_);
    socket_fd_ = other.socket_fd_;
    endpoint_valid_ = other.endpoint_valid_;
    manually_closed_ = other.manually_closed_;

    other.socket_fd_ = -1;
    other.endpoint_valid_ = false;
    other.manually_closed_ = true;

    return *this;
}

BackendSubmitResult KernelUdpExecutionBackend::submit(
    const BackendSubmitRequest& request
) {
    if (request.payload.empty()) {
        return reject(request, BackendRejectReason::empty_payload);
    }

    if (!endpoint_valid_) {
        return reject(request, BackendRejectReason::invalid_endpoint);
    }

    if (manually_closed_) {
        return reject(request, BackendRejectReason::backend_closed);
    }

    if (socket_fd_ < 0) {
        return reject(request, BackendRejectReason::socket_error);
    }

    const auto destination = make_destination(config_);

    const auto sent = ::sendto(
        socket_fd_,
        request.payload.data(),
        request.payload.size(),
        0,
        reinterpret_cast<const sockaddr*>(&destination),
        static_cast<socklen_t>(sizeof(destination))
    );

    if (sent < 0) {
        return reject(request, BackendRejectReason::send_failed);
    }

    const auto sent_size = static_cast<std::size_t>(sent);

    if (sent_size != request.payload.size()) {
        return reject(request, BackendRejectReason::send_failed);
    }

    return make_backend_accept(request);
}

bool KernelUdpExecutionBackend::open() const noexcept {
    return endpoint_valid_ && !manually_closed_ && socket_fd_ >= 0;
}

bool KernelUdpExecutionBackend::endpoint_valid() const noexcept {
    return endpoint_valid_;
}

int KernelUdpExecutionBackend::native_socket() const noexcept {
    return socket_fd_;
}

const std::string& KernelUdpExecutionBackend::destination_ipv4()
    const noexcept {
    return config_.destination_ipv4;
}

std::uint16_t KernelUdpExecutionBackend::destination_port() const noexcept {
    return config_.destination_port;
}

void KernelUdpExecutionBackend::close() noexcept {
    close_socket();
    manually_closed_ = true;
}

void KernelUdpExecutionBackend::open_socket() noexcept {
    if (!endpoint_valid_ || manually_closed_ || socket_fd_ >= 0) {
        return;
    }

    socket_fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
}

void KernelUdpExecutionBackend::close_socket() noexcept {
    if (socket_fd_ >= 0) {
        static_cast<void>(::close(socket_fd_));
        socket_fd_ = -1;
    }
}

BackendSubmitResult KernelUdpExecutionBackend::reject(
    const BackendSubmitRequest& request,
    BackendRejectReason reason
) const noexcept {
    return make_backend_reject(request, reason);
}

} // namespace fgep::execution