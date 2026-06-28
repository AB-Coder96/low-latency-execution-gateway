#pragma once

#include <cstdint>
#include <string_view>

namespace fgep::hardware {

enum class FpgaGateDecision {
    accept,
    reject,
    drop,
    pass_to_software,
    unavailable
};

enum class FpgaGateReason {
    none,
    hardware_unavailable,
    unsupported_packet,
    malformed_packet,
    config_miss,
    risk_reject,
    symbol_not_enabled,
    sequence_gap,
    software_required
};

enum class FpgaTimestampSource {
    none,
    software_reference,
    hardware_rx,
    hardware_gate
};

[[nodiscard]] constexpr std::string_view fpga_gate_decision_name(
    FpgaGateDecision decision
) noexcept {
    switch (decision) {
    case FpgaGateDecision::accept:
        return "accept";
    case FpgaGateDecision::reject:
        return "reject";
    case FpgaGateDecision::drop:
        return "drop";
    case FpgaGateDecision::pass_to_software:
        return "pass_to_software";
    case FpgaGateDecision::unavailable:
        return "unavailable";
    }

    return "unknown";
}

[[nodiscard]] constexpr std::string_view fpga_gate_reason_name(
    FpgaGateReason reason
) noexcept {
    switch (reason) {
    case FpgaGateReason::none:
        return "none";
    case FpgaGateReason::hardware_unavailable:
        return "hardware_unavailable";
    case FpgaGateReason::unsupported_packet:
        return "unsupported_packet";
    case FpgaGateReason::malformed_packet:
        return "malformed_packet";
    case FpgaGateReason::config_miss:
        return "config_miss";
    case FpgaGateReason::risk_reject:
        return "risk_reject";
    case FpgaGateReason::symbol_not_enabled:
        return "symbol_not_enabled";
    case FpgaGateReason::sequence_gap:
        return "sequence_gap";
    case FpgaGateReason::software_required:
        return "software_required";
    }

    return "unknown";
}

[[nodiscard]] constexpr std::string_view fpga_timestamp_source_name(
    FpgaTimestampSource source
) noexcept {
    switch (source) {
    case FpgaTimestampSource::none:
        return "none";
    case FpgaTimestampSource::software_reference:
        return "software_reference";
    case FpgaTimestampSource::hardware_rx:
        return "hardware_rx";
    case FpgaTimestampSource::hardware_gate:
        return "hardware_gate";
    }

    return "unknown";
}

struct FpgaGateCounters {
    std::uint64_t packets{};
    std::uint64_t accepted{};
    std::uint64_t rejected{};
    std::uint64_t dropped{};
    std::uint64_t passed_to_software{};
    std::uint64_t unavailable{};
    std::uint64_t malformed{};
};

struct FpgaGateResult {
    FpgaGateDecision decision{FpgaGateDecision::unavailable};
    FpgaGateReason reason{FpgaGateReason::hardware_unavailable};

    std::uint64_t sequence_number{};
    std::uint64_t receive_timestamp_ns{};
    std::uint64_t decision_timestamp_ns{};

    FpgaTimestampSource timestamp_source{FpgaTimestampSource::none};
    FpgaGateCounters counters{};

    [[nodiscard]] constexpr bool accepted() const noexcept {
        return decision == FpgaGateDecision::accept;
    }

    [[nodiscard]] constexpr bool rejected() const noexcept {
        return decision == FpgaGateDecision::reject;
    }

    [[nodiscard]] constexpr bool requires_software() const noexcept {
        return decision == FpgaGateDecision::pass_to_software
            || decision == FpgaGateDecision::unavailable;
    }

    [[nodiscard]] constexpr bool hardware_timestamp_available() const noexcept {
        return timestamp_source == FpgaTimestampSource::hardware_rx
            || timestamp_source == FpgaTimestampSource::hardware_gate;
    }
};

} // namespace fgep::hardware