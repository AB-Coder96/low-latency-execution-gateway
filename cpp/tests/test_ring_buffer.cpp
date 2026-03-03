#include "queues/ring_buffer_spsc.h"
#include "pipeline/pipeline.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>

using namespace zl;

TEST(SpscRingBuffer, PushPopSingleThread) {
  SpscRingBuffer<Message> q(8);
  Message m{};
  m.seq = 1;
  ASSERT_TRUE(q.try_push(m));
  Message out{};
  ASSERT_TRUE(q.try_pop(out));
  ASSERT_EQ(out.seq, 1u);
  ASSERT_FALSE(q.try_pop(out));
}

TEST(SpscRingBuffer, WrapAroundKeepsOrder) {
  SpscRingBuffer<Message> q(8);
  for (uint64_t i = 0; i < 1000; i++) {
    Message m{}; m.seq = i;
    while (!q.try_push(m)) {}
    Message out{};
    while (!q.try_pop(out)) {}
    ASSERT_EQ(out.seq, i);
  }
}

TEST(SpscRingBuffer, SpscTwoThreadNoLoss) {
  constexpr uint64_t N = 1'000'00;
  SpscRingBuffer<Message> q(1 << 12);

  std::atomic<bool> start{false};
  std::atomic<uint64_t> produced{0};
  std::atomic<uint64_t> consumed{0};

  std::thread prod([&]{
    while (!start.load(std::memory_order_acquire)) {}
    for (uint64_t i = 0; i < N; i++) {
      Message m{}; m.seq = i;
      while (!q.try_push(m)) {}
      produced.fetch_add(1, std::memory_order_relaxed);
    }
  });

  std::thread cons([&]{
    while (!start.load(std::memory_order_acquire)) {}
    uint64_t expected = 0;
    Message m{};
    while (expected < N) {
      if (q.try_pop(m)) {
        ASSERT_EQ(m.seq, expected);
        expected++;
        consumed.fetch_add(1, std::memory_order_relaxed);
      }
    }
  });

  start.store(true, std::memory_order_release);
  prod.join();
  cons.join();

  ASSERT_EQ(produced.load(), N);
  ASSERT_EQ(consumed.load(), N);
}
