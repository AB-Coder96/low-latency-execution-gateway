#include "fgep/execution/execution_backend.hpp"

namespace fgep::execution {

RecordingExecutionBackend::RecordingExecutionBackend(
    std::size_t max_submissions
) noexcept
    : max_submissions_{max_submissions} {
}

BackendSubmitResult RecordingExecutionBackend::submit(
    const BackendSubmitRequest& request
) {
    if (!open_) {
        return make_backend_reject(request, BackendRejectReason::backend_closed);
    }

    if (request.payload.empty()) {
        return make_backend_reject(request, BackendRejectReason::empty_payload);
    }

    if (at_capacity()) {
        return make_backend_reject(
            request,
            BackendRejectReason::capacity_exceeded
        );
    }

    submissions_.push_back(RecordedBackendSubmit{
        .user_ref_num = request.user_ref_num,
        .kind = request.kind,
        .payload = std::vector<std::byte>{
            request.payload.begin(),
            request.payload.end()
        }
    });

    return make_backend_accept(request);
}

void RecordingExecutionBackend::set_open(bool open) noexcept {
    open_ = open;
}

bool RecordingExecutionBackend::open() const noexcept {
    return open_;
}

std::size_t RecordingExecutionBackend::max_submissions() const noexcept {
    return max_submissions_;
}

void RecordingExecutionBackend::set_max_submissions(
    std::size_t max_submissions
) noexcept {
    max_submissions_ = max_submissions;
}

std::size_t RecordingExecutionBackend::submitted_count() const noexcept {
    return submissions_.size();
}

bool RecordingExecutionBackend::empty() const noexcept {
    return submissions_.empty();
}

const std::vector<RecordedBackendSubmit>&
RecordingExecutionBackend::submissions() const noexcept {
    return submissions_;
}

void RecordingExecutionBackend::clear() noexcept {
    submissions_.clear();
}

bool RecordingExecutionBackend::at_capacity() const noexcept {
    return max_submissions_ != 0 && submissions_.size() >= max_submissions_;
}

BackendSubmitResult make_backend_reject(
    const BackendSubmitRequest& request,
    BackendRejectReason reason
) noexcept {
    return BackendSubmitResult{
        .decision = BackendSubmitDecision::rejected,
        .reject_reason = reason,
        .user_ref_num = request.user_ref_num,
        .kind = request.kind,
        .payload_size = request.payload.size()
    };
}

BackendSubmitResult make_backend_accept(
    const BackendSubmitRequest& request
) noexcept {
    return BackendSubmitResult{
        .decision = BackendSubmitDecision::accepted,
        .reject_reason = BackendRejectReason::none,
        .user_ref_num = request.user_ref_num,
        .kind = request.kind,
        .payload_size = request.payload.size()
    };
}

} // namespace fgep::execution