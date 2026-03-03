# Perf tooling

This repo includes scripts and notes for Linux performance tooling that commonly shows up in low-latency work.

## Tools

### perf

- profile CPU cycles and hotspots:
  - `perf record ...`
  - `perf report`

Wrapper script:

```bash
./scripts/run_perf_record.sh out/perf_spsc_ring.data -- ./build/ZetaLatency_bench --scenario spsc --queue ring --seconds 10 --warmup 3 --out out/s.csv
```

### taskset

Pin a process to specific CPUs:

```bash
taskset -c 2,6 ./build/ZetaLatency_bench --scenario spsc --queue ring --seconds 10 --warmup 3 --out out/pinned.csv
```

### chrt (optional)

Run with realtime scheduling to reduce scheduling jitter (requires privileges):

```bash
sudo chrt -f 90 ./build/ZetaLatency_bench --scenario spsc --queue ring --seconds 10 --warmup 3 --out out/rt.csv
```

**Warning:** misuse can impact system responsiveness.

## perf_event permissions

The in-process `--perf` option uses Linux `perf_event_open` and may require configuration.
If it prints a permissions error, check:

- `/proc/sys/kernel/perf_event_paranoid`
- `/proc/sys/kernel/kptr_restrict`

You can also just use external `perf record` which is often simpler.
