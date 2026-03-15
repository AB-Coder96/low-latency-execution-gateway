# ZetaLatency — C++20 Lock-Free Pipelines & Tail Latency

ZetaLatency is a **benchmark-first C++20** project for measuring and understanding **tail latency** in real-time pipelines.
It focuses on the practical engineering work that low-latency / trading interviews care about:

- lock-free vs mutex-based queues
- cache alignment and false sharing
- CPU pinning and warm-up methodology
- NUMA effects and throughput vs jitter tradeoffs
- reproducible benchmark runs with **p50 / p99 / p99.9** reporting
- Linux-only perf options (optional)

> Primary stack: **C++20 + CMake + GoogleTest + (custom harness, optional Google Benchmark) + Python (pandas/matplotlib)**
>  
> Linux tooling: `perf`, `taskset`, `chrt` (optional), CPU governor notes.

---

## What ZetaLatency measures

### Queue implementations

- **Baseline**: `std::mutex` + `std::condition_variable` queue
- **SPSC**: ring buffer (lock-free, cacheline padded)
- **MPSC**: lock-free multi-producer single-consumer queue (Vyukov-style)

Variants:

- single-message dequeue
- (optional extension point) batch dequeue

### Pipeline shapes

- single producer → single consumer (**SPSC**)
- fan-in (**MPSC**) with multiple producers
- staged pipeline (**ingest → parse → book update**) with interchangeable queue types

### Output metrics

- throughput (msgs/sec)
- latency samples (ns) for histogramming
- percentiles: p50 / p95 / p99 / p99.9
- (optional) per-stage timing breakdown (pipeline scenario)
- (optional, Linux) perf counters (cycles/instructions) if enabled

---

## Reproducible methodology (what makes this credible)

ZetaLatency is designed so results are comparable across commits.

Checklist:

- pin threads to cores (affinity)
- fixed message sizes / fixed synthetic workload
- warm-up period before measurement
- release builds only (`-O3`; LTO optional)
- record machine + kernel + CPU governor
- export results to CSV for dashboards

See: `docs/methodology.md`.

---

## Quickstart

### Build

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

### Run benchmarks (examples)

```bash
./ZetaLatency_bench --scenario spsc --queue ring --seconds 30 --warmup 10 --out out/spsc_ring.csv
./ZetaLatency_bench --scenario spsc --queue mutex --seconds 30 --warmup 10 --out out/spsc_mutex.csv
```

Pin producer/consumer threads (Linux):

```bash
./ZetaLatency_bench --scenario spsc --queue ring --seconds 10 --warmup 3 --pin-producer 2 --pin-consumer 6 --out out/pinned.csv
```

### Plot (optional)

```bash
python3 python/plot_hist.py out/spsc_ring.csv
```

---

## Repo layout

```
ZetaLatency/
  cpp/
    common/
    queues/
    pipeline/
    bench/
    tests/
  python/
    plot_hist.py
  docs/
    methodology.md
    perf_tooling.md
    ci.md
    results.md
  scripts/
    run_benchmarks.sh
    system_info.sh
    run_perf_record.sh
    ci_nightly_perf.sh
  out/                      # ignored; generated
  CMakeLists.txt
  README.md
```

---

## What to publish on your portfolio site

Fill this with your measured numbers:

| Scenario | Queue | Throughput (msgs/s) | p50 | p99 | p99.9 | Notes |
|---|---|---:|---:|---:|---:|---|
| SPSC | ring | TBD | TBD | TBD | TBD | pinned core |
| SPSC | mutex | TBD | TBD | TBD | TBD | baseline |
| MPSC | lock-free | TBD | TBD | TBD | TBD | N producers |
| Pipeline | mixed | TBD | TBD | TBD | TBD | staged |

---

## Roadmap

- perf regression CI job (store last-known-good baseline)
- CPU governor check + warnings
- expand NUMA experiments and document results
- optional eBPF/perf integration scripts

# to add
market data receive latency

strategy decision latency

order serialization latency

wire-to-wire latency

exchange acknowledgment latency

cancel-to-ack latency

# Visulaization
dashboards for operators
latency heatmaps
percentile charts
venue-by-venue comparisons