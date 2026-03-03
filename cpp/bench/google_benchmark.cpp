#include "queues/ring_buffer_spsc.h"
#include "queues/mutex_queue.h"
#include "pipeline/pipeline.h"
#include <benchmark/benchmark.h>

using namespace zl;

static void BM_SpscRing(benchmark::State& state) {
  SpscRingBuffer<Message> q(1 << 16);
  Message m{};
  for (auto _ : state) {
    while (!q.try_push(m)) {}
    while (!q.try_pop(m)) {}
  }
}
BENCHMARK(BM_SpscRing);

static void BM_MutexQueue(benchmark::State& state) {
  MutexQueue<Message> q;
  Message m{};
  for (auto _ : state) {
    q.try_push(m);
    while (!q.try_pop(m)) {}
  }
}
BENCHMARK(BM_MutexQueue);

BENCHMARK_MAIN();
