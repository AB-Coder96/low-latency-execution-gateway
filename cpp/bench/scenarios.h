#pragma once
#include "bench/stats.h"
#include "bench/csv_writer.h"
#include "bench/work.h"
#include "common/affinity.h"
#include "common/sysinfo.h"
#include "common/time_ns.h"
#include "perf/perf_events.h"
#include "pipeline/pipeline.h"
#include "queues/mutex_queue.h"
#include "queues/ring_buffer_spsc.h"
#include "queues/mpsc_vyukov.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace zl {

struct RunConfig {
  std::string scenario = "spsc"; // spsc, mpsc, pipeline
  std::string queue = "ring";    // ring, mutex, mpsc
  int seconds = 10;
  int warmup = 3;
  int ring_capacity = 1 << 16;
  int producers = 4; // mpsc
  int pin_producer = -1;
  int pin_consumer = -1;
  int pin_parse = -1;
  int pin_book = -1;
  bool per_stage = false;
  bool perf = false;
  uint32_t parse_work = 200;
  uint32_t book_work = 200;
};

struct RunResult {
  double throughput = 0;
  Percentiles pct;
  PerfSnapshot perf{};
  bool perf_enabled = false;
};

inline uint64_t deadline_ns(uint64_t now, int seconds) {
  return now + static_cast<uint64_t>(seconds) * 1'000'000'000ull;
}

template <typename Q>
RunResult run_spsc(Q& q, const RunConfig& cfg, CsvWriter& csv) {
  std::atomic<bool> stop{false};
  std::atomic<uint64_t> produced{0};
  std::atomic<uint64_t> consumed{0};

  const uint64_t start_ns = now_ns();
  const uint64_t warmup_end = deadline_ns(start_ns, cfg.warmup);
  const uint64_t end_ns = deadline_ns(start_ns, cfg.warmup + cfg.seconds);

  std::vector<uint64_t> samples;
  samples.reserve(static_cast<size_t>(cfg.seconds) * 2'000'000); // best-effort reserve

  PerfEvents pe;
  std::string perr;
  bool perf_ok = cfg.perf && pe.start(&perr);

  std::thread prod([&]{
    if (cfg.pin_producer >= 0) pin_current_thread_to_cpu(cfg.pin_producer);
    Message m{};
    while (!stop.load(std::memory_order_relaxed)) {
      const uint64_t t = now_ns();
      if (t >= end_ns) break;
      m.seq = produced.fetch_add(1, std::memory_order_relaxed);
      m.t0_ns = t;
      while (!q.try_push(m)) {
        if (stop.load(std::memory_order_relaxed) || now_ns() >= end_ns) break;
        // spin
      }
    }
  });

  std::thread cons([&]{
    if (cfg.pin_consumer >= 0) pin_current_thread_to_cpu(cfg.pin_consumer);
    Message m{};
    while (true) {
      if (q.try_pop(m)) {
        const uint64_t t = now_ns();
        consumed.fetch_add(1, std::memory_order_relaxed);
        if (t >= warmup_end && t < end_ns) {
          samples.push_back(t - m.t0_ns);
        }
        if (t >= end_ns) break;
      } else {
        const uint64_t t = now_ns();
        if (t >= end_ns) break;
        // spin
      }
    }
    stop.store(true, std::memory_order_relaxed);
  });

  cons.join();
  stop.store(true, std::memory_order_relaxed);
  prod.join();

  RunResult rr{};
  rr.pct = compute_percentiles(samples);
  rr.throughput = samples.empty() ? 0.0 : (static_cast<double>(samples.size()) / static_cast<double>(cfg.seconds));
  rr.perf_enabled = perf_ok;
  if (perf_ok) rr.perf = pe.stop(&perr);

  // write CSV
  csv.write_header("latency_ns");
  for (auto ns : samples) csv.write_row(ns);

  // stdout summary
  std::cout << "Throughput (samples/sec): " << rr.throughput << "\n";
  std::cout << "p50=" << rr.pct.p50 << " ns, p95=" << rr.pct.p95
            << " ns, p99=" << rr.pct.p99 << " ns, p99.9=" << rr.pct.p999 << " ns\n";
  if (cfg.perf) {
    if (perf_ok) {
      std::cout << "perf cycles=" << rr.perf.cycles << " instructions=" << rr.perf.instructions << "\n";
    } else {
      std::cout << "perf disabled: " << perr << "\n";
    }
  }
  return rr;
}


inline RunResult run_spsc_mutex(MutexQueue<Message>& q, const RunConfig& cfg, CsvWriter& csv) {
  std::atomic<bool> stop{false};
  std::atomic<uint64_t> produced{0};

  const uint64_t start_ns = now_ns();
  const uint64_t warmup_end = deadline_ns(start_ns, cfg.warmup);
  const uint64_t end_ns = deadline_ns(start_ns, cfg.warmup + cfg.seconds);

  std::vector<uint64_t> samples;
  samples.reserve(static_cast<size_t>(cfg.seconds) * 2'000'000);

  std::thread prod([&]{
    if (cfg.pin_producer >= 0) pin_current_thread_to_cpu(cfg.pin_producer);
    Message m{};
    while (!stop.load(std::memory_order_relaxed)) {
      const uint64_t t = now_ns();
      if (t >= end_ns) break;
      m.seq = produced.fetch_add(1, std::memory_order_relaxed);
      m.t0_ns = t;
      q.try_push(m);
    }
    q.close();
  });

  std::thread cons([&]{
    if (cfg.pin_consumer >= 0) pin_current_thread_to_cpu(cfg.pin_consumer);
    Message m{};
    while (q.pop_blocking(m)) {
      const uint64_t t = now_ns();
      if (t >= warmup_end && t < end_ns) samples.push_back(t - m.t0_ns);
      if (t >= end_ns) break;
    }
    stop.store(true, std::memory_order_relaxed);
  });

  cons.join();
  stop.store(true, std::memory_order_relaxed);
  prod.join();

  RunResult rr{};
  rr.pct = compute_percentiles(samples);
  rr.throughput = samples.empty() ? 0.0 : (static_cast<double>(samples.size()) / static_cast<double>(cfg.seconds));

  csv.write_header("latency_ns");
  for (auto ns : samples) csv.write_row(ns);

  std::cout << "Throughput (samples/sec): " << rr.throughput << "\n";
  std::cout << "p50=" << rr.pct.p50 << " ns, p95=" << rr.pct.p95
            << " ns, p99=" << rr.pct.p99 << " ns, p99.9=" << rr.pct.p999 << " ns\n";
  return rr;
}


inline RunResult run_mpsc(const RunConfig& cfg, CsvWriter& csv) {
  MpscQueue<Message> q;
  std::atomic<bool> stop{false};
  std::atomic<uint64_t> produced{0};
  std::atomic<uint64_t> consumed{0};

  const uint64_t start_ns = now_ns();
  const uint64_t warmup_end = deadline_ns(start_ns, cfg.warmup);
  const uint64_t end_ns = deadline_ns(start_ns, cfg.warmup + cfg.seconds);

  std::vector<uint64_t> samples;
  samples.reserve(static_cast<size_t>(cfg.seconds) * 2'000'000);

  std::vector<std::thread> producers;
  producers.reserve(cfg.producers);
  for (int i = 0; i < cfg.producers; i++) {
    producers.emplace_back([&, i]{
      // Spread producers across cores if requested, else no pin.
      if (cfg.pin_producer >= 0) pin_current_thread_to_cpu(cfg.pin_producer + i);
      Message m{};
      while (!stop.load(std::memory_order_relaxed)) {
        const uint64_t t = now_ns();
        if (t >= end_ns) break;
        m.seq = produced.fetch_add(1, std::memory_order_relaxed);
        m.t0_ns = t;
        q.try_push(m);
      }
    });
  }

  std::thread consumer([&]{
    if (cfg.pin_consumer >= 0) pin_current_thread_to_cpu(cfg.pin_consumer);
    Message m{};
    while (true) {
      if (q.try_pop(m)) {
        const uint64_t t = now_ns();
        consumed.fetch_add(1, std::memory_order_relaxed);
        if (t >= warmup_end && t < end_ns) {
          samples.push_back(t - m.t0_ns);
        }
        if (t >= end_ns) break;
      } else {
        const uint64_t t = now_ns();
        if (t >= end_ns) break;
      }
    }
    stop.store(true, std::memory_order_relaxed);
  });

  consumer.join();
  stop.store(true, std::memory_order_relaxed);
  for (auto& t : producers) t.join();

  RunResult rr{};
  rr.pct = compute_percentiles(samples);
  rr.throughput = samples.empty() ? 0.0 : (static_cast<double>(samples.size()) / static_cast<double>(cfg.seconds));

  csv.write_header("latency_ns");
  for (auto ns : samples) csv.write_row(ns);

  std::cout << "Producers: " << cfg.producers << "\n";
  std::cout << "Throughput (samples/sec): " << rr.throughput << "\n";
  std::cout << "p50=" << rr.pct.p50 << " ns, p95=" << rr.pct.p95
            << " ns, p99=" << rr.pct.p99 << " ns, p99.9=" << rr.pct.p999 << " ns\n";

  return rr;
}

template <typename Q1, typename Q2>
RunResult run_pipeline(Q1& q1, Q2& q2, const RunConfig& cfg, CsvWriter& csv) {
  std::atomic<bool> stop{false};
  std::atomic<uint64_t> produced{0};

  const uint64_t start_ns = now_ns();
  const uint64_t warmup_end = deadline_ns(start_ns, cfg.warmup);
  const uint64_t end_ns = deadline_ns(start_ns, cfg.warmup + cfg.seconds);

  struct Row { uint64_t end_to_end, parse_stage, book_stage; };
  std::vector<Row> rows;
  rows.reserve(static_cast<size_t>(cfg.seconds) * 1'000'000);

  std::thread ingest([&]{
    if (cfg.pin_producer >= 0) pin_current_thread_to_cpu(cfg.pin_producer);
    Message m{};
    while (!stop.load(std::memory_order_relaxed)) {
      const uint64_t t = now_ns();
      if (t >= end_ns) break;
      m.seq = produced.fetch_add(1, std::memory_order_relaxed);
      m.t0_ns = t;
      while (!q1.try_push(m)) {
        if (stop.load(std::memory_order_relaxed)) break;
      }
    }
  });

  std::thread parse([&]{
    if (cfg.pin_parse >= 0) pin_current_thread_to_cpu(cfg.pin_parse);
    Message m{};
    while (true) {
      if (q1.try_pop(m)) {
        burn_cycles(cfg.parse_work);
        m.t_parse_done_ns = now_ns();
        while (!q2.try_push(m)) {
          if (stop.load(std::memory_order_relaxed)) break;
        }
      } else {
        const uint64_t t = now_ns();
        if (t >= end_ns) break;
      }
    }
  });

  std::thread book([&]{
    if (cfg.pin_book >= 0) pin_current_thread_to_cpu(cfg.pin_book);
    Message m{};
    while (true) {
      if (q2.try_pop(m)) {
        burn_cycles(cfg.book_work);
        const uint64_t t = now_ns();
        if (t >= warmup_end && t < end_ns) {
          uint64_t parse_stage = m.t_parse_done_ns > m.t0_ns ? (m.t_parse_done_ns - m.t0_ns) : 0;
          uint64_t book_stage = t > m.t_parse_done_ns ? (t - m.t_parse_done_ns) : 0;
          rows.push_back({t - m.t0_ns, parse_stage, book_stage});
        }
        if (t >= end_ns) break;
      } else {
        const uint64_t t = now_ns();
        if (t >= end_ns) break;
      }
    }
    stop.store(true, std::memory_order_relaxed);
  });

  book.join();
  stop.store(true, std::memory_order_relaxed);
  ingest.join();
  parse.join();

  std::vector<uint64_t> e2e;
  e2e.reserve(rows.size());
  for (auto& r : rows) e2e.push_back(r.end_to_end);

  RunResult rr{};
  rr.pct = compute_percentiles(e2e);
  rr.throughput = rows.empty() ? 0.0 : (static_cast<double>(rows.size()) / static_cast<double>(cfg.seconds));

  if (cfg.per_stage) {
    csv.write_header("end_to_end_ns,parse_stage_ns,book_stage_ns");
    for (auto& r : rows) csv.write_row(r.end_to_end, r.parse_stage, r.book_stage);
  } else {
    csv.write_header("latency_ns");
    for (auto& r : rows) csv.write_row(r.end_to_end);
  }

  std::cout << "Throughput (samples/sec): " << rr.throughput << "\n";
  std::cout << "p50=" << rr.pct.p50 << " ns, p95=" << rr.pct.p95
            << " ns, p99=" << rr.pct.p99 << " ns, p99.9=" << rr.pct.p999 << " ns\n";
  return rr;
}

} // namespace zl
