#include "fgep/execution/execution_backend.hpp"

#include <array>
#include <cassert>
#include <cstddef>

namespace {

[[nodiscard]] std::array<std::byte, 4> payload() noexcept {
    return {
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04}
    };
}

} // namespace

int main() {
    using namespace fgep::execution;

    {
        const auto bytes = payload();

        const BackendSubmitRequest request{
            .user_ref_num = 10,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        };

        const auto rejected = make_backend_reject(
            request,
            BackendRejectReason::backend_closed
        );

        assert(rejected.rejected());
        assert(!rejected.accepted());
        assert(rejected.reject_reason == BackendRejectReason::backend_closed);
        assert(rejected.user_ref_num == 10);
        assert(rejected.kind == BackendOrderKind::enter);
        assert(rejected.payload_size == bytes.size());

        const auto accepted = make_backend_accept(request);

        assert(accepted.accepted());
        assert(!accepted.rejected());
        assert(accepted.reject_reason == BackendRejectReason::none);
        assert(accepted.user_ref_num == 10);
        assert(accepted.kind == BackendOrderKind::enter);
        assert(accepted.payload_size == bytes.size());
    }

    {
        RecordingExecutionBackend backend{};

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

        assert(!backend.empty());
        assert(backend.submitted_count() == 1);
        assert(backend.submissions().size() == 1);
        assert(backend.submissions()[0].user_ref_num == 1);
        assert(backend.submissions()[0].kind == BackendOrderKind::enter);
        assert(backend.submissions()[0].payload.size() == bytes.size());
        assert(backend.submissions()[0].payload[0] == std::byte{0x01});
        assert(backend.submissions()[0].payload[3] == std::byte{0x04});
    }

    {
        RecordingExecutionBackend backend{};

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 2,
            .kind = BackendOrderKind::cancel,
            .payload = {}
        });

        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::empty_payload);
        assert(result.user_ref_num == 2);
        assert(result.kind == BackendOrderKind::cancel);
        assert(result.payload_size == 0);
        assert(backend.empty());
    }

    {
        RecordingExecutionBackend backend{};

        backend.set_open(false);
        assert(!backend.open());

        const auto bytes = payload();

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 3,
            .kind = BackendOrderKind::replace,
            .payload = bytes
        });

        assert(result.rejected());
        assert(result.reject_reason == BackendRejectReason::backend_closed);
        assert(result.user_ref_num == 3);
        assert(result.kind == BackendOrderKind::replace);
        assert(backend.empty());

        backend.set_open(true);
        assert(backend.open());
    }

    {
        RecordingExecutionBackend backend{1};

        assert(backend.max_submissions() == 1);

        const auto bytes = payload();

        const auto first = backend.submit(BackendSubmitRequest{
            .user_ref_num = 4,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

        const auto second = backend.submit(BackendSubmitRequest{
            .user_ref_num = 5,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

        assert(first.accepted());
        assert(second.rejected());
        assert(second.reject_reason == BackendRejectReason::capacity_exceeded);
        assert(backend.submitted_count() == 1);

        backend.set_max_submissions(2);

        const auto third = backend.submit(BackendSubmitRequest{
            .user_ref_num = 6,
            .kind = BackendOrderKind::cancel,
            .payload = bytes
        });

        assert(third.accepted());
        assert(backend.submitted_count() == 2);
    }

    {
        RecordingExecutionBackend backend{};

        const auto bytes = payload();

        const auto result = backend.submit(BackendSubmitRequest{
            .user_ref_num = 7,
            .kind = BackendOrderKind::enter,
            .payload = bytes
        });

        assert(result.accepted());
        assert(backend.submitted_count() == 1);

        backend.clear();

        assert(backend.empty());
        assert(backend.submitted_count() == 0);
    }

    return 0;
}