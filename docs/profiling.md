# Profiling

This document describes how to generate local CPU flame graphs for FGEP binaries.

The profiling flow is intentionally manual. It is not run as part of normal builds, tests, or benchmark reports.

## Requirements

The flame graph script requires:

- Linux `perf`
- `stackcollapse-perf.pl`
- `flamegraph.pl`

The two Perl scripts come from Brendan Gregg's FlameGraph tools and must be available on `PATH`.

Example check:

```bash
command -v perf
command -v stackcollapse-perf.pl
command -v flamegraph.pl