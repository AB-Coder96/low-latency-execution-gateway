#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  scripts/profile_flamegraph.sh <binary> [output_root] [-- <binary args...>]

Examples:
  scripts/profile_flamegraph.sh build/profile/apps/fgep_benchmark_report
  scripts/profile_flamegraph.sh build/profile/apps/fgep_benchmark_report reports/profile
  scripts/profile_flamegraph.sh build/profile/apps/fgep_benchmark_report reports/profile -- reports/profile-input

Output layout:
  reports/profile/<timestamp>/perf.data
  reports/profile/<timestamp>/perf.script
  reports/profile/<timestamp>/out.folded
  reports/profile/<timestamp>/flamegraph.svg

Requirements:
  - Linux perf
  - stackcollapse-perf.pl from Brendan Gregg's FlameGraph tools
  - flamegraph.pl from Brendan Gregg's FlameGraph tools

The FlameGraph scripts must be available on PATH.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if [[ $# -lt 1 ]]; then
    usage >&2
    exit 2
fi

binary="$1"
shift

output_root="reports/profile"

if [[ $# -gt 0 && "${1:-}" != "--" ]]; then
    output_root="$1"
    shift
fi

if [[ $# -gt 0 && "${1:-}" == "--" ]]; then
    shift
fi

if [[ ! -x "$binary" ]]; then
    echo "error: binary is not executable: $binary" >&2
    exit 1
fi

if ! command -v perf >/dev/null 2>&1; then
    echo "error: perf was not found on PATH" >&2
    exit 1
fi

if ! command -v stackcollapse-perf.pl >/dev/null 2>&1; then
    echo "error: stackcollapse-perf.pl was not found on PATH" >&2
    echo "Install Brendan Gregg's FlameGraph tools and add them to PATH." >&2
    exit 1
fi

if ! command -v flamegraph.pl >/dev/null 2>&1; then
    echo "error: flamegraph.pl was not found on PATH" >&2
    echo "Install Brendan Gregg's FlameGraph tools and add them to PATH." >&2
    exit 1
fi

timestamp="$(date '+%Y-%m-%d_%H%M%S')"
output_dir="$output_root/$timestamp"

mkdir -p "$output_dir"

perf_data="$output_dir/perf.data"
perf_script="$output_dir/perf.script"
folded_output="$output_dir/out.folded"
flamegraph_svg="$output_dir/flamegraph.svg"

echo "Profiling binary: $binary"
echo "Output root: $output_root"
echo "Output directory: $output_dir"

perf record -F 997 -g --call-graph fp \
    -o "$perf_data" \
    "$binary" "$@"

perf script -i "$perf_data" > "$perf_script"
stackcollapse-perf.pl "$perf_script" > "$folded_output"
flamegraph.pl "$folded_output" > "$flamegraph_svg"

echo "Wrote:"
echo "  $perf_data"
echo "  $perf_script"
echo "  $folded_output"
echo "  $flamegraph_svg"