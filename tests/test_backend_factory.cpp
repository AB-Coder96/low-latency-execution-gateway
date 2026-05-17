#include "fgep/execution/backend_factory.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>

namespace {

[[nodiscard]] std::array<std::byte, 4> payload() noexcept {
    return {
        std::byte{0x42},
        std::byte{0x41},
        std::byte{0x43},
        std::byte{0x4b}
    };
}

} // namespace

int main() {
    using namespace fgep::execution;

    {
        assert(
            backend_technology_from_string("recording")
            == BackendTechnology::recording
        );
        assert(
            backend_technology_from_string("kernel_udp")
            == BackendTechnology::kernel_udp
        );
        assert(
            backend_technology_from_string("afxdp")
            == BackendTechnology::afxdp
        );
        assert(
            backend_technology_from_string("dpdk")
            == BackendTechnology::dpdk
        );
        assert(
            backend_technology_from_string("unknown")
            == BackendTechnology::kernel_udp
        );

        assert(
            std::string_view{
                backend_technology_name(BackendTechnology::recording)
            } == "recording"
        );
        assert(
            std::string_view{
                backend_technology_name(BackendTechnology::kernel_udp)
            } == "kernel_udp"
        );
        assert(
            std::string_view{
                backend_technology_name(BackendTechnology::afxdp)
            } == "afxdp"
        );
        assert(
            std::string_view{
                backend_technology_name(BackendTechnology::dpdk)
            } == "dpdk"
        );
    }

    {
        const auto backend = make_execution_backend(BackendFactoryConfig{
            .technology = BackendTechnology::recording,
            .kernel_udp = {},
            .afxdp = {},
            .dpdk = {},
            .recording_max_submissions = 2
        });

        assert(backend != nullptr);

        const auto bytes = payload();

        const auto result = backend->submit(BackendSubmitRequest{
            .user_ref_num = 1,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

        assert(result.accepted());
    }

    {
        const auto backend = make_execution_backend(BackendFactoryConfig{
            .technology = BackendTechnology::kernel_udp,
            .kernel_udp = KernelUdpBackendConfig{
                .destination_ipv4 = "127.0.0.1",
                .destination_port = 49'999
            },
            .afxdp = {},
            .dpdk = {},
            .recording_max_submissions = 0
        });

        assert(backend != nullptr);

        const auto bytes = payload();

        const auto result = backend->submit(BackendSubmitRequest{
            .user_ref_num = 2,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

        assert(result.accepted());
    }

    {
        const auto backend = make_execution_backend(BackendFactoryConfig{
            .technology = BackendTechnology::afxdp,
            .kernel_udp = {},
            .afxdp = AfXdpBackendConfig{
                .interface_name = "eth0",
                .queue_id = 0,
                .umem_frame_count = 4096,
                .umem_frame_size = 2048,
                .tx_batch_size = 64
            },
            .dpdk = {},
            .recording_max_submissions = 0
        });

        assert(backend != nullptr);

        const auto bytes = payload();

        const auto result = backend->submit(BackendSubmitRequest{
            .user_ref_num = 3,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::unsupported_backend);
    }

    {
        const auto backend = make_execution_backend(BackendFactoryConfig{
            .technology = BackendTechnology::dpdk,
            .kernel_udp = {},
            .afxdp = {},
            .dpdk = DpdkBackendConfig{},
            .recording_max_submissions = 0
        });

        assert(backend != nullptr);

        const auto bytes = payload();

        const auto result = backend->submit(BackendSubmitRequest{
            .user_ref_num = 4,
            .kind = BackendOrderKind::enter,
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

    static_cast<void>(default_backend_technology());

    return 0;
}