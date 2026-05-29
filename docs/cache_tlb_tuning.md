# Cache and TLB Tuning

This document describes the project policy for cache-aware and TLB-aware runtime work.

The policy is measurement first.

Do not add alignment, prefetching, huge pages, or custom arenas everywhere without evidence.

## Goals

Runtime data layout should reduce:

```text
cache misses
false sharing
unnecessary pointer chasing
TLB misses
runtime page faults
allocator overhead
```

The main goal is predictable runtime behavior, not just a single fastest microbenchmark result.

## Cache-line separation

Use cache-line separation for data that is written by different threads.

Good candidates:

```text
SPSC producer index
SPSC consumer index
runtime atomic counters
high-frequency queue statistics
shutdown/control flags near hot data
```

Avoid adding `alignas(64)` to every structure.

Alignment can increase memory footprint and hurt cache locality when overused.

## False sharing policy

Avoid placing independently written fields on the same cache line.

Example risk:

```text
producer writes head
consumer writes tail
head and tail share one cache line
```

Preferred layout:

```text
producer-owned index on one cache line
consumer-owned index on another cache line
read-mostly config separate from write-heavy counters
```

## Data locality

Prefer compact fixed storage for hot runtime data.

Good candidates:

```text
fixed order tables
direct-index instrument directories
fixed price level arrays
inline payload buffers
fixed latency sample arrays
```

Avoid hot-path pointer chasing where possible:

```text
std::map nodes
unordered_map rehashing
heap-owned payload objects
linked structures
```

## TLB policy

Preallocating memory does not guarantee runtime pages are resident.

Runtime memory that must be available without page faults should eventually be:

```text
allocated during initialization
prefaulted during warmup
optionally locked on Linux
measured with page-fault counters
```

Future Linux-specific tuning may include:

```text
mlockall(MCL_CURRENT | MCL_FUTURE)
madvise(..., MADV_HUGEPAGE)
huge-page-advised runtime arenas
NUMA-aware allocation
```

These should remain optional and measured.

## Prefaulting

Prefault utilities should touch memory before runtime begins.

Target structures:

```text
packet buffers
fixed queues
fixed order tables
fixed price levels
latency recorders
runtime arenas
```

Prefaulting should happen in warmup, not in the running phase.

## Huge pages

Huge pages are not an early default.

Use huge pages only after measuring TLB pressure.

Useful evidence:

```text
dTLB load misses
dTLB MPKI
page faults
cycles per operation
latency tail changes
```

Huge-page behavior is platform-specific and may not improve small working sets.

## Hardware counters

Optional hardware counters should eventually track:

```text
cycles
instructions
cache references
cache misses
branches
branch misses
dTLB load misses
page faults
```

Derived metrics should include:

```text
cache_miss_rate
cache_mpki
dtlb_mpki
cycles_per_operation
instructions_per_operation
```

## Prefetching policy

Manual prefetch is a late-stage optimization.

Do not prefetch by default.

Consider prefetch only when:

```text
the access pattern is predictable
hardware counters show cache-miss pressure
a benchmark shows stable improvement
the code remains readable
```

## Measurement checklist

Before accepting a cache/TLB optimization, collect:

```text
benchmark name
build preset
CPU model
kernel version
sample count or operation count
baseline result
optimized result
cache/TLB counters if available
notes about noise and repeatability
```

## Non-goals for early commits

Do not start with:

```text
huge pages everywhere
manual prefetch everywhere
alignas(64) everywhere
custom allocators everywhere
mutexes around all maps
claims that synthetic latency is real network latency
```

Optimization should follow correctness tests, no-heap tests, and repeatable benchmarks.