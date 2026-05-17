#include "fgep/execution/kernel_udp_backend.hpp"

#include <array>
#include <cassert>
#include <cstddef>

namespace {

[[nodiscard]] std::array<std::byte, 4> payload() noexcept {
    return {
        std::byte{0x4f},
        std::byte{0x55},
        std::byte{0x43},
        std::byte{0x48}
    };
}

} // namespace

int main() {
    using namespace fgep::execution;

    {
        KernelUdpExecutionBackend backend{KernelUdpBackendConfig{
            .destination_ipv4 = "127.0.0.1",
            .destination_port = 49'999
        }};

        assert(backend.endpoint_valid());
        assert(backend.open());
        assert(backend.native_socket() >= 0);
        assert(backend.destination_ipv4() == "127.0.0.1");
        assert(backend.destination_port() == 49'999);

        const auto bytes = payload();

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 1,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

        assert(result.accepted());
        assert(result.reject_reason == BackendRejectReason::none);
        assert(result.user_ref_num == 1);
        assert(result.kind == BackendOrderKind::enter);
        assert(result.payload_size == bytes.size());
    }

    {
        KernelUdpExecutionBackend backend{KernelUdpBackendConfig{
            .destination_ipv4 = "not-an-ip",
            .destination_port = 49'999
        }};

        assert(!backend.endpoint_valid());
        assert(!backend.open());

        const auto bytes = payload();

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 2,
            .kind = BackendOrderKind::cancel,
            .payload = bytes
        });

        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::invalid_endpoint);
        assert(result.user_ref_num == 2);
        assert(result.kind == BackendOrderKind::cancel);
    }

    {
        KernelUdpExecutionBackend backend{KernelUdpBackendConfig{
            .destination_ipv4 = "127.0.0.1",
            .destination_port = 0
        }};

        assert(!backend.endpoint_valid());
        assert(!backend.open());

        const auto bytes = payload();

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 3,
            .kind = BackendOrderKind::replace,
            .payload = bytes
        });

        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::invalid_endpoint);
    }

    {
        KernelUdpExecutionBackend backend{KernelUdpBackendConfig{
            .destination_ipv4 = "127.0.0.1",
            .destination_port = 49'999
        }};

        backend.close();

        assert(!backend.open());

        const auto bytes = payload();

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 4,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::backend_closed);
    }

    {
        KernelUdpExecutionBackend backend{KernelUdpBackendConfig{
            .destination_ipv4 = "127.0.0.1",
            .destination_port = 49'999
        }};

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 5,
            .kind = BackendOrderKind::enter,
            .payload = {}
        });

        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::empty_payload);
        assert(result.payload_size == 0);
    }

    {
        KernelUdpExecutionBackend first{KernelUdpBackendConfig{
            .destination_ipv4 = "127.0.0.1",
            .destination_port = 49'999
        }};

        assert(first.open());

        KernelUdpExecutionBackend second{std::move(first)};

        assert(second.open());
        assert(!first.open());

        const auto bytes = payload();

        const auto result = second.submit(BackendSubmitRequest{
            .user_ref_num = 6,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

        assert(result.accepted());
    }

    return 0;
}