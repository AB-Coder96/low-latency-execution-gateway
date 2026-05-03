#pragma once

#include "fgep/core/types.hpp"

#include <chrono>
#include <cstdint>

namespace fgep {

enum class TimestampSource : std::uint8_t {
    software_steady_clock,
    nic_hardware,
    fpga_hardware,
    ptp_clock
};

// -----------------------------------------------------------------------------
// Clock selection
// -----------------------------------------------------------------------------
//
//  monotonic SteadyClock is used for elapsed-time measurement.

using SteadyClock = std::chrono::steady_clock;
using TimePoint = SteadyClock::time_point;

// -----------------------------------------------------------------------------
// Timestamp helpers
// -----------------------------------------------------------------------------

[[nodiscard]] inline TimestampNs now_ns() noexcept {
    const auto now = SteadyClock::now().time_since_epoch();

    return static_cast<TimestampNs>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count()
    );
}

[[nodiscard]] inline TimestampNs to_ns(TimePoint time_point) noexcept {
    return static_cast<TimestampNs>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            time_point.time_since_epoch()
        ).count()
    );
}

[[nodiscard]] inline TimestampNs elapsed_ns(
    TimePoint start,
    TimePoint end
) noexcept {
    return static_cast<TimestampNs>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
    );
}

[[nodiscard]] inline TimestampNs elapsed_since_ns(TimePoint start) noexcept {
    return elapsed_ns(start, SteadyClock::now());
}

[[nodiscard]] constexpr bool is_valid_timestamp(TimestampNs timestamp) noexcept {
    return timestamp > 0;
}

[[nodiscard]] constexpr bool is_hardware_timestamp_source(
    TimestampSource source
) noexcept {
    return source == TimestampSource::nic_hardware
        || source == TimestampSource::fpga_hardware
        || source == TimestampSource::ptp_clock;
}
} // namespace fgep