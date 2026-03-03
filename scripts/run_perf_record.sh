#!/usr/bin/env bash
set -euo pipefail

# Example wrapper to capture perf profiles for a benchmark run.
# Requires Linux perf tooling and permissions.
#
# Usage:
#   ./scripts/run_perf_record.sh out/perf_spsc_ring.data -- ./build/ZetaLatency_bench --scenario spsc --queue ring --seconds 10 --warmup 3 --out out/s.csv

OUT_DATA="$1"
shift

if ! command -v perf >/dev/null 2>&1; then
  echo "ERROR: perf not installed"
  exit 2
fi

echo "[perf] recording to $OUT_DATA"
perf record -o "$OUT_DATA" --call-graph fp "$@"
echo "[perf] done. You can inspect with: perf report -i $OUT_DATA"
