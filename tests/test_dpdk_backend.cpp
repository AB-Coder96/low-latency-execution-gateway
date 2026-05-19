#include "fgep/execution/dpdk_backend.hpp"

#include <array>
#include <cassert>
#include <cstddef>

namespace {

[[nodiscard]] std::array<std::byte, 4> payload() noexcept {
    return {
        std::byte{0x44},
        std::byte{0x50},
        std::byte{0x44},
        std::byte{0x4b}
    };
}

} // namespace

int main() {
    using namespace fgep::execution;

    {
        DpdkExecutionBackend backend{};

        assert(backend.configured());
        assert(!backend.open());
        assert(backend.port_id() == 0);
        assert(backend.socket_id() == -1);
        assert(backend.queue_id() == 0);
        assert(backend.burst_size() == 32);
        assert(backend.mbuf_pool_size() == 8192);

        const auto bytes = payload();

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 1,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

#if FGEP_HAVE_DPDK
        assert(
            result.reject_reason == BackendRejectReason::backend_closed
            || result.reject_reason == BackendRejectReason::send_failed
            || result.accepted()
        );
#else
        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::unsupported_backend);
#endif
    }

    {
        DpdkExecutionBackend backend{DpdkBackendConfig{
            .eal_args = {"fgep-dpdk", "-l", "0-1", "-n", "4"},
            .port_id = 0,
            .queue_id = 0,
            .socket_id = 0,
            .tx_desc_count = 1024,
            .burst_size = 32,
            .mbuf_pool_size = 8192,
            .mbuf_cache_size = 250,
            .destination_mac = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}},
            .source_mac = {{0x02, 0x00, 0x00, 0x00, 0x00, 0x01}},
            .ether_type = 0x88B5
        }};

        assert(backend.configured());
        assert(backend.port_id() == 0);
        assert(backend.queue_id() == 0);
        assert(backend.socket_id() == 0);
        assert(backend.config().eal_args.size() == 5);

        const auto bytes = payload();

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 2,
            .kind = BackendOrderKind::replace,
            .payload = bytes
        });

#if FGEP_HAVE_DPDK
        assert(
            result.accepted()
            || result.reject_reason == BackendRejectReason::backend_closed
            || result.reject_reason == BackendRejectReason::send_failed
        );
#else
        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::unsupported_backend);
#endif
    }

    {
        DpdkExecutionBackend backend{DpdkBackendConfig{}};

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
        DpdkExecutionBackend backend{DpdkBackendConfig{
            .eal_args = {},
            .port_id = 0,
            .queue_id = 0,
            .tx_desc_count = 1024,
            .burst_size = 32,
            .mbuf_pool_size = 8192,
            .mbuf_cache_size = 250
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
        DpdkExecutionBackend backend{DpdkBackendConfig{
            .eal_args = {"fgep-dpdk"},
            .port_id = 0,
            .queue_id = 0,
            .tx_desc_count = 0,
            .burst_size = 32,
            .mbuf_pool_size = 8192,
            .mbuf_cache_size = 250
        }};

        assert(!backend.configured());
    }

    {
        DpdkExecutionBackend backend{DpdkBackendConfig{
            .eal_args = {"fgep-dpdk"},
            .port_id = 0,
            .queue_id = 0,
            .tx_desc_count = 1024,
            .burst_size = 0,
            .mbuf_pool_size = 8192,
            .mbuf_cache_size = 250
        }};

        assert(!backend.configured());
    }

{
    DpdkExecutionBackend backend{DpdkBackendConfig{
        .eal_args = {"fgep-dpdk"},
        .port_id = 0,
        .queue_id = 0,
        .socket_id = -2,
        .tx_desc_count = 1024,
        .burst_size = 32,
        .mbuf_pool_size = 8192,
        .mbuf_cache_size = 250
    }};

    assert(!backend.configured());
}

    return 0;
}