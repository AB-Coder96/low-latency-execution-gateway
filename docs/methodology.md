# Methodology

ZetaLatency tries to make benchmark results **reproducible across runs and across commits**.

## Core principles

1. **Pin threads to cores** (affinity) to reduce OS scheduling noise.
2. **Warm-up before measuring** to amortize cold caches, page faults, and CPU frequency ramp.
3. **Release builds only** (`-O3` / `Release`). Consider LTO once baseline is stable.
4. **Fixed message sizes / stable work** (synthetic compute) so comparisons stay meaningful.
5. **Record the environment** (kernel, CPU model, CPU governor, perf settings).

## Warm-up

Bench runs use two phases:

- warmup phase (default: 3s): samples are discarded
- measurement phase (default: 10s): samples are recorded to CSV

## CPU pinning

The benchmark binary supports pinning on Linux:

- `--pin-producer <cpu>`
- `--pin-consumer <cpu>`
- `--pin-parse <cpu>`, `--pin-book <cpu>` for the pipeline scenario

Alternatively (and sometimes preferable), wrap the whole process with `taskset`:

```bash
taskset -c 2,6 ./ZetaLatency_bench --scenario spsc --queue ring --seconds 10 --warmup 3 --out out/pinned.csv
```

## CPU governor

The scaling governor can strongly affect tail latency.

Recommended for benchmarking (if your system supports it):

- `performance` governor
- disable turbo (optional for stability)

Check:

```bash
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
```

ZetaLatency writes `cpu_governor` into the CSV metadata when available.

## NUMA notes

NUMA can dominate results:

- cross-socket producer/consumer increases latency and jitter
- memory allocation locality matters

Use `numactl` to constrain placement (optional), and document exactly what you did.

## What to report

When publishing results, include:

- commit SHA
- CPU model + kernel
- governor state
- pinning strategy
- run length + warmup
- percentiles (p50/p95/p99/p99.9) + throughput
