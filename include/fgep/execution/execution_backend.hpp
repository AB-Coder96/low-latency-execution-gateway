#pragma once

#include "fgep/ouch/ouch_types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace fgep::execution {

enum class BackendOrderKind : std::uint8_t {
    enter,
    replace,
    cancel
};

enum class BackendSubmitDecision : std::uint8_t {
    accepted,
    rejected
};

enum class BackendRejectReason : std::uint8_t {
    none,
    backend_closed,
    empty_payload,
    capacity_exceeded,
    invalid_endpoint,
    socket_error,
    send_failed
};

struct BackendSubmitRequest {
    ouch::UserRefNum user_ref_num{};
    BackendOrderKind kind{BackendOrderKind::enter};
    std::span<const std::byte> payload{};
};

struct BackendSubmitResult {
    BackendSubmitDecision decision{BackendSubmitDecision::rejected};
    BackendRejectReason reject_reason{BackendRejectReason::none};
    ouch::UserRefNum user_ref_num{};
    BackendOrderKind kind{BackendOrderKind::enter};
    std::size_t payload_size{};

    [[nodiscard]] constexpr bool accepted() const noexcept {
        return decision == BackendSubmitDecision::accepted;
    }

    [[nodiscard]] constexpr bool rejected() const noexcept {
        return decision == BackendSubmitDecision::rejected;
    }
};

class ExecutionBackend {
public:
    virtual ~ExecutionBackend() = default;

    [[nodiscard]] virtual BackendSubmitResult submit(
        const BackendSubmitRequest& request
    ) = 0;
};

struct RecordedBackendSubmit {
    ouch::UserRefNum user_ref_num{};
    BackendOrderKind kind{BackendOrderKind::enter};
    std::vector<std::byte> payload{};
};

class RecordingExecutionBackend final : public ExecutionBackend {
public:
    RecordingExecutionBackend() = default;

    explicit RecordingExecutionBackend(std::size_t max_submissions) noexcept;

    [[nodiscard]] BackendSubmitResult submit(
        const BackendSubmitRequest& request
    ) override;

    void set_open(bool open) noexcept;
    [[nodiscard]] bool open() const noexcept;

    [[nodiscard]] std::size_t max_submissions() const noexcept;
    void set_max_submissions(std::size_t max_submissions) noexcept;

    [[nodiscard]] std::size_t submitted_count() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] const std::vector<RecordedBackendSubmit>& submissions()
        const noexcept;

    void clear() noexcept;

private:
    bool open_{true};
    std::size_t max_submissions_{};
    std::vector<RecordedBackendSubmit> submissions_{};

    [[nodiscard]] bool at_capacity() const noexcept;
};

[[nodiscard]] BackendSubmitResult make_backend_reject(
    const BackendSubmitRequest& request,
    BackendRejectReason reason
) noexcept;

[[nodiscard]] BackendSubmitResult make_backend_accept(
    const BackendSubmitRequest& request
) noexcept;

} // namespace fgep::execution