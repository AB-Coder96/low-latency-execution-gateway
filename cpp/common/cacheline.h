#pragma once
#include <cstddef>
#include <new>

namespace zl {

#ifndef ZL_CACHELINE_SIZE
// 64 is the de-facto cacheline for x86_64; keep configurable.
static constexpr std::size_t kCacheLine = 64;
#else
static constexpr std::size_t kCacheLine = ZL_CACHELINE_SIZE;
#endif

// Forces `value` onto its own cacheline and ensures the overall object is at least one cacheline in size.
// For types that are already a multiple of cacheline size, we add an extra cacheline of padding
// (wastes space, but avoids zero-length arrays and keeps separation properties simple).
template <typename T>
struct alignas(kCacheLine) CacheAligned {
  T value;
  static constexpr std::size_t kPad =
      ((kCacheLine - (sizeof(T) % kCacheLine)) % kCacheLine) ? ((kCacheLine - (sizeof(T) % kCacheLine)) % kCacheLine) : kCacheLine;
  unsigned char pad[kPad] = {};
};

} // namespace zl
