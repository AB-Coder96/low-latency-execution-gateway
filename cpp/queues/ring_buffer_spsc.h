#pragma once
#include "common/cacheline.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>
#include <stdexcept>

namespace zl {

// Lock-free SPSC ring buffer.
// - capacity must be power-of-two.
// - head/tail are separated onto distinct cachelines to reduce false sharing.
template <typename T>
class SpscRingBuffer {
 public:
  explicit SpscRingBuffer(std::size_t capacity_pow2)
      : mask_(capacity_pow2 - 1),
        buf_(capacity_pow2) {
    // capacity must be power-of-two
    if ((capacity_pow2 & mask_) != 0) {
      throw std::runtime_error("SpscRingBuffer capacity must be power-of-two");
    }
  }

  bool try_push(const T& v) noexcept {
    const uint64_t head = head_.value.load(std::memory_order_relaxed);
    const uint64_t next = head + 1;
    if (next - tail_cache_ > buf_.size()) {
      tail_cache_ = tail_.value.load(std::memory_order_acquire);
      if (next - tail_cache_ > buf_.size()) return false; // full
    }
    buf_[head & mask_] = v;
    head_.value.store(next, std::memory_order_release);
    return true;
  }

  bool try_pop(T& out) noexcept {
    const uint64_t tail = tail_.value.load(std::memory_order_relaxed);
    if (tail == head_cache_) {
      head_cache_ = head_.value.load(std::memory_order_acquire);
      if (tail == head_cache_) return false; // empty
    }
    out = buf_[tail & mask_];
    tail_.value.store(tail + 1, std::memory_order_release);
    return true;
  }

  std::size_t capacity() const noexcept { return buf_.size(); }

 private:
  const uint64_t mask_;
  std::vector<T> buf_;

  CacheAligned<std::atomic<uint64_t>> head_{std::atomic<uint64_t>{0}};
  CacheAligned<std::atomic<uint64_t>> tail_{std::atomic<uint64_t>{0}};

  // Per-thread caches (no atomics needed)
  uint64_t head_cache_ = 0;
  uint64_t tail_cache_ = 0;
};

} // namespace zl
