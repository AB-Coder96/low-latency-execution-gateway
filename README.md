# C++20 Low-Latency Execution Platform

A C++20 low-latency trading-infrastructure prototype with protocol parsing, market-data replay, order-book state, risk/execution logic, guardrail gating, kernel UDP execution, a DPDK-capable execution backend, cache-aware SPSC execution handoff, and reproducible EC2/WSL performance reporting.

The project demonstrates systems engineering across binary protocols, execution-path design, Linux networking, DPDK/ENA setup, concurrency, cache alignment, NUMA-aware memory setup, and performance measurement.

---

## Highlights

| Area |
|---|
| C++20 / CMake / Ninja build |
| ITCH / OUCH / MoldUDP64 codecs | 
| Market-data replay pipeline | 
| Venue-local books and normalized BBO |
| Risk supervisor and order lifecycle simulation | 
| Software guardrail and permissive gate | 
| Kernel UDP execution backend | 
| DPDK-capable execution backend | 
| AWS EC2 ENA PMD validation | 
| Hugepage setup on EC2 | 
| Secondary ENI DPDK binding workflow | 
| NUMA-aware DPDK mbuf setup |
| Cache-aligned SPSC ring | 
| Acquire/release SPSC stress tests |
| SPSC backend-submit queue | 
| Queued / async execution handoff |
| Cache-aligned CoreStats counters | 
| Linux `perf stat` capture script |  
| Timestamped perf report generator | 
| EC2 vs WSL SPSC benchmark report | 

---

## Architecture

The project models a low-latency software execution path:

```text
MoldUDP64 / ITCH-style input
    -> decode / replay
    -> venue-local books
    -> normalized BBO
    -> order candidate generation
    -> risk supervisor
    -> OUCH lifecycle simulation
    -> guardrail / permissive gate
    -> execution backend
    -> telemetry and benchmark reports
```

Execution backends are exposed through a common interface:

```text
ExecutionBackend
    -> RecordingExecutionBackend
    -> KernelUdpExecutionBackend
    -> DpdkExecutionBackend
```

The current execution handoff path uses:

```text
producer thread
    -> BackendSubmitQueue
    -> cache-aligned SPSC ring
    -> queued / async execution backend
    -> backend submit path
```

---
## Runtime performance architecture docs

The runtime performance roadmap is documented before deeper runtime rewrites begin:

```text
docs/runtime_memory_model.md
docs/runtime_threading_model.md
docs/cache_tlb_tuning.md
docs/network_benchmarking.md
docs/profiling.md
```

These documents describe:

```text
initialization vs runtime phase boundaries
no-heap-after-initialization goals
single-owner threaded state
queue-based cross-thread handoff
allowed atomic usage
cache/TLB tuning policy
profiling workflow
network benchmark limitations
```


## Repository layout

```text
include/fgep/
  bench/        Benchmark configs, reports, and system metadata
  bbo/          Normalized best-bid-offer logic
  book/         Venue-local and multi-venue books
  core/         Core types, errors, SPSC ring, time utilities
  execution/   Order lifecycle, backend interfaces, kernel UDP, DPDK, queued backend
  gate/         Guardrail state, control path, permissive gate
  instrument/  Instrument/reference-data types
  itch/         ITCH messages and codecs
  moldudp64/   MoldUDP64 messages and codecs
  ouch/         OUCH messages and codecs
  replay/      Replay pipeline
  risk/         Risk supervisor
  telemetry/   Pipeline events, latency summaries, CoreStats
  venue/        Venue/MIC types

src/            Implementations
tests/          Unit and integration tests
apps/           CLI benchmark and report applications
scripts/        Build, test, and perf helpers
docs/           Methodology and setup notes
reports/        Timestamped benchmark outputs
```

---

## Build and test

Debug build:

```bash
./scripts/build.sh debug
./scripts/test.sh debug
```

Current debug validation:

```text
100% tests passed, 0 tests failed out of 46
```

Manual CMake flow:

```bash
cmake --preset debug
cmake --build --preset debug -j2
ctest --test-dir build/debug --output-on-failure
```

Release build:

```bash
cmake --preset release
cmake --build --preset release -j2
ctest --test-dir build/release --output-on-failure
```

---

## DPDK / EC2 validation

The EC2 setup uses a safe two-interface workflow:

```text
Primary ENI:
    stays on the kernel ENA driver for SSH and default route

Secondary ENI:
    used for DPDK / VFIO experiments
```

Validated EC2 runtime components:

```text
Hugepages mounted under /mnt/huge
Secondary ENI bound to vfio-pci
DPDK ENA PMD loaded as net_ena
ENA PMD devarg: llq_policy=1
Port link up
RX/TX queues configured
NUMA socket 0 detected
testpmd starts and runs on the secondary ENI
```

The DPDK backend includes:

```text
rte_eal_init
rte_pktmbuf_pool_create
rte_eth_dev_configure
rte_eth_tx_queue_setup
rte_eth_dev_start
rte_pktmbuf_alloc
rte_pktmbuf_append
Ethernet header + payload copy
rte_eth_tx_burst
cleanup / RAII
```

The mbuf pool and TX queue setup are socket-aware:

```text
explicit config socket
    -> NIC port socket
    -> current lcore socket
    -> socket 0 fallback
```

---

## SPSC execution handoff

Core SPSC implementation:

```text
include/fgep/core/spsc_ring.hpp
tests/test_spsc_ring.cpp
```

Execution-path queueing:

```text
include/fgep/execution/backend_submit_queue.hpp
include/fgep/execution/queued_execution_backend.hpp
include/fgep/execution/async_execution_backend.hpp
```

Design properties:

```text
fixed capacity
power-of-two queue size
cache-line aligned queue object
separate producer and consumer state
release-store publication
acquire-load consumption
payload-owning backend queue entries
no dynamic allocation in push/pop
wraparound/full/empty tests
cross-thread stress tests
```

The backend submit queue copies payload bytes into a fixed-size queue entry before handoff. This keeps queue entries self-contained and safe across producer/consumer threads.

---

## CoreStats telemetry

CoreStats counters are cache-line aligned and use relaxed atomics for low-overhead hot-path accounting.

Implemented counters include:

```text
rx_packets
tx_packets
rx_dropped
tx_dropped
rx_nombuf
submit_queued
submit_rejected
ring_full
backend_accepted
backend_rejected
```

Test coverage includes:

```text
alignment checks
named counter checks
snapshot checks
reset checks
cross-thread scrape checks
```

---

## Perf tooling

Perf capture script:

```text
scripts/run_perf_stat.sh
```

SPSC benchmark driver:

```text
apps/fgep_spsc_queue_perf.cpp
```

SPSC report generator:

```text
apps/fgep_spsc_perf_report.cpp
```

The report generator parses raw `perf stat` output and app-level benchmark output, then produces a timestamped Markdown report with:

```text
input metadata
host/kernel metadata
queue run summary
perf counters
derived metrics
system metadata
```

Report filenames are timestamped automatically:

```text
reports/perf/ec2-aws-ena-dpdk-spsc-queue-report-YYYYMMDD-HHMMSS.md
reports/perf/wsl-spsc-queue-report-YYYYMMDD-HHMMSS.md
```

---

## SPSC benchmark

Benchmark command:

```bash
./build/debug/apps/fgep_spsc_queue_perf --payload-size 64
```

Capture command:

```bash
./scripts/run_perf_stat.sh \
  --cmd './build/debug/apps/fgep_spsc_queue_perf --payload-size 64' \
  --warmup 2 \
  --seconds 10 \
  --output reports/perf/<label>-spsc-queue-perf-final.txt
```

Report generation:

```bash
./build/debug/apps/fgep_spsc_perf_report \
  --input reports/perf/<label>-spsc-queue-perf-final.txt \
  --label <label>
```

A valid report contains nonzero values for:

```text
produced
consumed
throughput_handoffs_per_sec
cycles_per_handoff
instructions_per_handoff
```

---

## EC2 vs WSL SPSC benchmark results

Current debug-mode validation results:

| Metric | EC2 AWS Linux | WSL2 | Winner |
|---|---:|---:|---|
| Throughput, handoffs/sec | 10,934,182.93 | 4,811,138.34 | EC2 |
| Throughput, M handoffs/sec | 10.93 | 4.81 | EC2 |
| Cycles / handoff | 521.88 | 919.44 | EC2 |
| Instructions / handoff | 1149.80 | 1197.49 | EC2 |

Derived comparison:

| Metric | Result |
|---|---:|
| EC2 throughput uplift vs WSL | 2.27x |
| EC2 cycles/handoff reduction vs WSL | 43.24% |
| EC2 instructions/handoff reduction vs WSL | 3.98% |

Interpretation:

```text
EC2 Linux delivered substantially higher SPSC execution-handoff throughput than WSL2 for the same benchmark, payload size, warmup, duration, and report flow.
```

---

## Evidence produced

Current evidence artifacts include:

```text
DPDK ENA/testpmd proof on EC2
valid EC2 SPSC perf report
valid WSL SPSC perf report
EC2 vs WSL summary report
46 passing debug tests
timestamped perf reports with system metadata
```

## next benchmark

A useful follow-up benchmark is:

```text
BackendSubmitQueue
    -> DpdkExecutionBackend::submit()
    -> rte_eth_tx_burst()
    -> ENA TX queue
```

This would measure the cost of extending the current SPSC execution handoff into the DPDK TX path on EC2.

Another follow-up comparison is:

```text
KernelUdpExecutionBackend on EC2
vs
DpdkExecutionBackend on EC2
```

using the same payload size, build type, duration, warmup, and perf reporting workflow.
