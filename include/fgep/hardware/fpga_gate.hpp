#pragma once

#include "fgep/hardware/fpga_gate_result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace fgep::hardware {

struct FpgaPacketMetadata {
    std::uint64_t sequence_number{};
    std::uint64_t receive_timestamp_ns{};

    std::uint32_t packet_length{};
    std::uint32_t stock_locate{};

    bool has_instrument_key{};
    bool has_order_fields{};

    std::uint8_t side{};
    std::uint32_t quantity{};
    std::uint32_t price{};
};

struct FpgaGateRequest {
    std::span<const std::byte> packet{};
    FpgaPacketMetadata metadata{};
};

class FpgaGate {
public:
    virtual ~FpgaGate() = default;

    FpgaGate(const FpgaGate&) = delete;
    FpgaGate& operator=(const FpgaGate&) = delete;

    FpgaGate(FpgaGate&&) = delete;
    FpgaGate& operator=(FpgaGate&&) = delete;

    [[nodiscard]] virtual bool available() const noexcept = 0;

    [[nodiscard]] virtual FpgaGateResult evaluate(
        const FpgaGateRequest& request
    ) noexcept = 0;

    [[nodiscard]] virtual FpgaGateCounters counters() const noexcept = 0;

protected:
    FpgaGate() = default;
};

class UnavailableFpgaGate final : public FpgaGate {
public:
    [[nodiscard]] bool available() const noexcept override {
        return false;
    }

    [[nodiscard]] FpgaGateResult evaluate(
        const FpgaGateRequest& request
    ) noexcept override {
        ++counters_.packets;
        ++counters_.unavailable;

        return FpgaGateResult{
            .decision = FpgaGateDecision::unavailable,
            .reason = FpgaGateReason::hardware_unavailable,
            .sequence_number = request.metadata.sequence_number,
            .receive_timestamp_ns = request.metadata.receive_timestamp_ns,
            .decision_timestamp_ns = 0U,
            .timestamp_source = FpgaTimestampSource::none,
            .counters = counters_
        };
    }

    [[nodiscard]] FpgaGateCounters counters() const noexcept override {
        return counters_;
    }

private:
    FpgaGateCounters counters_{};
};

} // namespace fgep::hardware