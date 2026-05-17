#include "fgep/execution/afxdp_backend.hpp"

#include <array>
#include <cassert>
#include <cstddef>

namespace {

[[nodiscard]] std::array<std::byte, 4> payload() noexcept {
    return {
        std::byte{0x41},
        std::byte{0x46},
        std::byte{0x58},
        std::byte{0x44}
    };
}

} // namespace

int main() {
    using namespace fgep::execution;

    {
        AfXdpExecutionBackend backend{};

        assert(!backend.configured());
        assert(!backend.open());
        assert(backend.interface_name().empty());
        assert(backend.queue_id() == 0);

        const auto bytes = payload();

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 1,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::invalid_endpoint);
        assert(result.user_ref_num == 1);
        assert(result.kind == BackendOrderKind::enter);
        assert(result.payload_size == bytes.size());
    }

    {
        AfXdpExecutionBackend backend{AfXdpBackendConfig{
            .interface_name = "eth0",
            .queue_id = 0,
            .umem_frame_count = 4096,
            .umem_frame_size = 2048,
            .tx_batch_size = 64
        }};

        assert(backend.configured());
        assert(!backend.open());
        assert(backend.interface_name() == "eth0");
        assert(backend.queue_id() == 0);
        assert(backend.config().umem_frame_count == 4096);
        assert(backend.config().umem_frame_size == 2048);
        assert(backend.config().tx_batch_size == 64);

        const auto bytes = payload();

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 2,
            .kind = BackendOrderKind::replace,
            .payload = bytes
        });

        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::unsupported_backend);
        assert(result.user_ref_num == 2);
        assert(result.kind == BackendOrderKind::replace);
        assert(result.payload_size == bytes.size());
    }

    {
        AfXdpExecutionBackend backend{AfXdpBackendConfig{
            .interface_name = "eth0",
            .queue_id = 1,
            .umem_frame_count = 4096,
            .umem_frame_size = 2048,
            .tx_batch_size = 64
        }};

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 3,
            .kind = BackendOrderKind::cancel,
            .payload = {}
        });

        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::empty_payload);
        assert(result.user_ref_num == 3);
        assert(result.kind == BackendOrderKind::cancel);
        assert(result.payload_size == 0);
    }

    {
        AfXdpExecutionBackend backend{AfXdpBackendConfig{
            .interface_name = "eth0",
            .queue_id = 0,
            .umem_frame_count = 0,
            .umem_frame_size = 2048,
            .tx_batch_size = 64
        }};

        assert(!backend.configured());

        const auto bytes = payload();

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 4,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::invalid_endpoint);
    }

    {
        AfXdpExecutionBackend backend{AfXdpBackendConfig{
            .interface_name = "eth0",
            .queue_id = 0,
            .umem_frame_count = 4096,
            .umem_frame_size = 1024,
            .tx_batch_size = 64
        }};

        assert(!backend.configured());
    }

    {
        AfXdpExecutionBackend backend{AfXdpBackendConfig{
            .interface_name = "eth0",
            .queue_id = 0,
            .umem_frame_count = 4096,
            .umem_frame_size = 2048,
            .tx_batch_size = 0
        }};

        assert(!backend.configured());
    }

    return 0;
}