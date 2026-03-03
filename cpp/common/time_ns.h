#pragma once
#include <cstdint>
#include <chrono>

#if defined(__linux__)
  #include <time.h>
#endif

namespace zl {

inline uint64_t now_ns() noexcept {
#if defined(__linux__)
  timespec ts{};
  // MONOTONIC_RAW is stable for benchmarking; falls back to MONOTONIC if unavailable.
  #ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  #else
    clock_gettime(CLOCK_MONOTONIC, &ts);
  #endif
  return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ull + static_cast<uint64_t>(ts.tv_nsec);
#else
  using clock = std::chrono::steady_clock;
  auto t = clock::now().time_since_epoch();
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t).count());
#endif
}

} // namespace zl
