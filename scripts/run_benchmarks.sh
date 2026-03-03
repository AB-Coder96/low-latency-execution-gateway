#!/usr/bin/env bash
set -euo pipefail

# Run a small suite of benchmarks and store CSVs under out/<timestamp>/
#
# Usage:
#   ./scripts/run_benchmarks.sh [--seconds 30] [--warmup 10] [--prefix out/run1]
#
# Notes:
# - For pinning, you can prefer taskset externally:
#     taskset -c 2,6 ./build/ZetaLatency_bench ...
# - For realtime scheduling (optional, needs privileges):
#     sudo chrt -f 90 ./build/ZetaLatency_bench ...

SECONDS=10
WARMUP=3
PREFIX="out/$(date +%Y%m%d_%H%M%S)"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --seconds) SECONDS="$2"; shift 2;;
    --warmup)  WARMUP="$2"; shift 2;;
    --prefix)  PREFIX="$2"; shift 2;;
    *) echo "Unknown arg: $1"; exit 2;;
  esac
done

mkdir -p "$PREFIX"

echo "[info] writing system info"
./scripts/system_info.sh > "$PREFIX/system_info.txt" || true

BIN="./build/ZetaLatency_bench"
if [[ ! -x "$BIN" ]]; then
  BIN="./build/Release/ZetaLatency_bench"
fi
if [[ ! -x "$BIN" ]]; then
  echo "ERROR: benchmark binary not found. Build first (see README)."
  exit 2
fi

echo "[run] spsc ring"
"$BIN" --scenario spsc --queue ring  --seconds "$SECONDS" --warmup "$WARMUP" --out "$PREFIX/spsc_ring.csv"

echo "[run] spsc mutex"
"$BIN" --scenario spsc --queue mutex --seconds "$SECONDS" --warmup "$WARMUP" --out "$PREFIX/spsc_mutex.csv"

echo "[run] mpsc (4 producers)"
"$BIN" --scenario mpsc --queue mpsc --producers 4 --seconds "$SECONDS" --warmup "$WARMUP" --out "$PREFIX/mpsc_4p.csv"

echo "[run] pipeline ring->ring (per-stage)"
"$BIN" --scenario pipeline --queue ring,ring --seconds "$SECONDS" --warmup "$WARMUP" --per-stage --out "$PREFIX/pipeline_ring_ring.csv"

echo "[done] outputs in $PREFIX"
