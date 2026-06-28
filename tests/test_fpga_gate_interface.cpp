#include "fgep/hardware/fpga_gate.hpp"
#include "fgep/hardware/fpga_gate_result.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <type_traits>

int main() {
    using namespace fgep::hardware;

    static_assert(std::is_polymorphic_v<FpgaGate>);
    static_assert(std::is_final_v<UnavailableFpgaGate>);

    {
        assert(fpga_gate_decision_name(FpgaGateDecision::accept) == "accept");
        assert(fpga_gate_decision_name(FpgaGateDecision::reject) == "reject");
        assert(fpga_gate_decision_name(FpgaGateDecision::drop) == "drop");
        assert(
            fpga_gate_decision_name(FpgaGateDecision::pass_to_software)
                == "pass_to_software"
        );
        assert(
            fpga_gate_decision_name(FpgaGateDecision::unavailable)
                == "unavailable"
        );
    }

    {
        assert(fpga_gate_reason_name(FpgaGateReason::none) == "none");
        assert(
            fpga_gate_reason_name(FpgaGateReason::hardware_unavailable)
                == "hardware_unavailable"
        );
        assert(
            fpga_gate_reason_name(FpgaGateReason::unsupported_packet)
                == "unsupported_packet"
        );
        assert(
            fpga_gate_reason_name(FpgaGateReason::malformed_packet)
                == "malformed_packet"
        );
        assert(
            fpga_gate_reason_name(FpgaGateReason::config_miss)
                == "config_miss"
        );
        assert(
            fpga_gate_reason_name(FpgaGateReason::risk_reject)
                == "risk_reject"
        );
        assert(
            fpga_gate_reason_name(FpgaGateReason::symbol_not_enabled)
                == "symbol_not_enabled"
        );
        assert(
            fpga_gate_reason_name(FpgaGateReason::sequence_gap)
                == "sequence_gap"
        );
        assert(
            fpga_gate_reason_name(FpgaGateReason::software_required)
                == "software_required"
        );
    }

    {
        assert(
            fpga_timestamp_source_name(FpgaTimestampSource::none)
                == "none"
        );
        assert(
            fpga_timestamp_source_name(FpgaTimestampSource::software_reference)
                == "software_reference"
        );
        assert(
            fpga_timestamp_source_name(FpgaTimestampSource::hardware_rx)
                == "hardware_rx"
        );
        assert(
            fpga_timestamp_source_name(FpgaTimestampSource::hardware_gate)
                == "hardware_gate"
        );
    }

    {
        FpgaGateResult result{
            .decision = FpgaGateDecision::accept,
            .reason = FpgaGateReason::none,
            .sequence_number = 7U,
            .receive_timestamp_ns = 100U,
            .decision_timestamp_ns = 120U,
            .timestamp_source = FpgaTimestampSource::hardware_gate,
            .counters = {}
        };

        assert(result.accepted());
        assert(!result.rejected());
        assert(!result.requires_software());
        assert(result.hardware_timestamp_available());
    }

    {
        FpgaGateResult result{
            .decision = FpgaGateDecision::pass_to_software,
            .reason = FpgaGateReason::software_required,
            .sequence_number = 8U,
            .receive_timestamp_ns = 200U,
            .decision_timestamp_ns = 0U,
            .timestamp_source = FpgaTimestampSource::none,
            .counters = {}
        };

        assert(!result.accepted());
        assert(!result.rejected());
        assert(result.requires_software());
        assert(!result.hardware_timestamp_available());
    }

    {
        std::array<std::byte, 4> packet{
            std::byte{0x01},
            std::byte{0x02},
            std::byte{0x03},
            std::byte{0x04}
        };

        UnavailableFpgaGate gate{};

        assert(!gate.available());
        assert(gate.counters().packets == 0U);
        assert(gate.counters().unavailable == 0U);

        const FpgaGateRequest request{
            .packet = packet,
            .metadata = FpgaPacketMetadata{
                .sequence_number = 42U,
                .receive_timestamp_ns = 1'000U,
                .packet_length = static_cast<std::uint32_t>(packet.size()),
                .stock_locate = 1U,
                .has_instrument_key = true,
                .has_order_fields = false,
                .side = 0U,
                .quantity = 0U,
                .price = 0U
            }
        };

        const auto result = gate.evaluate(request);

        assert(result.decision == FpgaGateDecision::unavailable);
        assert(result.reason == FpgaGateReason::hardware_unavailable);
        assert(result.sequence_number == 42U);
        assert(result.receive_timestamp_ns == 1'000U);
        assert(result.decision_timestamp_ns == 0U);
        assert(result.requires_software());
        assert(!result.hardware_timestamp_available());

        assert(result.counters.packets == 1U);
        assert(result.counters.unavailable == 1U);

        assert(gate.counters().packets == 1U);
        assert(gate.counters().unavailable == 1U);
    }

    return 0;
}