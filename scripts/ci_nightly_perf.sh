#!/usr/bin/env bash
set -euo pipefail

# CI-friendly perf baseline run (kept short).
# Produces out/ci_<timestamp>/*.csv which GitHub Actions uploads as artifacts.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DZL_BUILD_TESTS=OFF
cmake --build build -j

STAMP="$(date +%Y%m%d_%H%M%S)"
OUT_DIR="out/ci_${STAMP}"
mkdir -p "$OUT_DIR"

./build/ZetaLatency_bench --scenario spsc --queue ring --seconds 5 --warmup 2 --out "$OUT_DIR/spsc_ring.csv"
./build/ZetaLatency_bench --scenario spsc --queue mutex --seconds 5 --warmup 2 --out "$OUT_DIR/spsc_mutex.csv"
./build/ZetaLatency_bench --scenario mpsc --queue mpsc --producers 4 --seconds 5 --warmup 2 --out "$OUT_DIR/mpsc_4p.csv"

echo "Wrote: $OUT_DIR"
