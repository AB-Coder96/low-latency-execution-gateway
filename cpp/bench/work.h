#pragma once
#include <cstdint>

namespace zl {

// Cheap deterministic work to model "parse/book update" and introduce variability.
// Intentionally simple and stable across platforms.
//
// We keep a data dependency and use an asm barrier to discourage the compiler from
// optimizing the loop away.
inline void burn_cycles(uint32_t iters) {
  uint64_t x = 0x9e3779b97f4a7c15ull;
  for (uint32_t i = 0; i < iters; i++) {
    x ^= x << 7;
    x ^= x >> 9;
    x *= 0xbf58476d1ce4e5b9ull;
  }
#if defined(__GNUC__) || defined(__clang__)
  asm volatile("" : "+r"(x) : : "memory");
#endif
  (void)x;
}

} // namespace zl
