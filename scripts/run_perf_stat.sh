#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "usage:"
  echo "  $0 --pid <pid> [--seconds 30] [--output reports/perf/perf-stat.txt]"
  echo "  $0 --cmd '<command>' [--warmup 5] [--seconds 30] [--output reports/perf/perf-stat.txt]"
  echo
  echo "examples:"
  echo "  $0 --pid 1234 --seconds 30 --output reports/perf/ec2-perf-stat.txt"
  echo "  $0 --cmd './build/debug/apps/fgep_spsc_queue_perf --payload-size 64' --warmup 2 --seconds 10"
}

PID=""
CMD=""
SECONDS_TO_RUN="30"
WARMUP_SECONDS="5"
OUTPUT="reports/perf/perf-stat.txt"
CPU_LIST=""
APP_OUTPUT=""
CHILD_PID=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --pid)
      PID="${2:-}"
      shift 2
      ;;
    --cmd)
      CMD="${2:-}"
      shift 2
      ;;
    --seconds)
      SECONDS_TO_RUN="${2:-}"
      shift 2
      ;;
    --warmup)
      WARMUP_SECONDS="${2:-}"
      shift 2
      ;;
    --output)
      OUTPUT="${2:-}"
      shift 2
      ;;
    --cpus)
      CPU_LIST="${2:-}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "unknown argument: $1"
      usage
      exit 1
      ;;
  esac
done

if [[ -n "$PID" && -n "$CMD" ]]; then
  echo "error: choose either --pid or --cmd, not both"
  exit 1
fi

if [[ -z "$PID" && -z "$CMD" ]]; then
  echo "error: missing --pid or --cmd"
  usage
  exit 1
fi

mkdir -p "$(dirname "$OUTPUT")"

EVENTS="cycles,instructions,branches,branch-misses,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,dTLB-loads,dTLB-load-misses"

{
  echo "# perf stat capture"
  echo "# date_utc: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "# host: $(hostname)"
  echo "# kernel: $(uname -a)"
  echo "# events: $EVENTS"
  echo "# seconds: $SECONDS_TO_RUN"
  echo "# warmup_seconds: $WARMUP_SECONDS"
  echo "# cpu_list: ${CPU_LIST:-all/process-default}"
  echo
} | tee "$OUTPUT"

if [[ -n "$PID" ]]; then
  if ! kill -0 "$PID" 2>/dev/null; then
    echo "error: pid $PID does not exist" | tee -a "$OUTPUT"
    exit 1
  fi

  echo "# mode: attach" | tee -a "$OUTPUT"
  echo "# pid: $PID" | tee -a "$OUTPUT"
  echo | tee -a "$OUTPUT"

  if [[ -n "$CPU_LIST" ]]; then
    sudo perf stat -C "$CPU_LIST" \
      -e "$EVENTS" \
      -- sleep "$SECONDS_TO_RUN" \
      2>&1 | tee -a "$OUTPUT"
  else
    sudo perf stat -p "$PID" \
      -e "$EVENTS" \
      -- sleep "$SECONDS_TO_RUN" \
      2>&1 | tee -a "$OUTPUT"
  fi

  exit 0
fi

echo "# mode: command" | tee -a "$OUTPUT"
echo "# command: $CMD" | tee -a "$OUTPUT"
echo | tee -a "$OUTPUT"

APP_OUTPUT="$(mktemp)"

cleanup() {
  if [[ -n "${CHILD_PID:-}" ]] && kill -0 "$CHILD_PID" 2>/dev/null; then
    kill "$CHILD_PID" 2>/dev/null || true
    wait "$CHILD_PID" 2>/dev/null || true
  fi

  if [[ -n "${APP_OUTPUT:-}" && -f "$APP_OUTPUT" ]]; then
    rm -f "$APP_OUTPUT"
  fi
}

trap cleanup EXIT INT TERM

bash -lc "exec $CMD" > "$APP_OUTPUT" 2>&1 &
CHILD_PID=$!

sleep "$WARMUP_SECONDS"

if ! kill -0 "$CHILD_PID" 2>/dev/null; then
  {
    echo "error: command exited before perf measurement started"
    echo
    echo "# app output"
    cat "$APP_OUTPUT"
  } | tee -a "$OUTPUT"

  exit 1
fi

if [[ -n "$CPU_LIST" ]]; then
  sudo perf stat -C "$CPU_LIST" \
    -e "$EVENTS" \
    -- sleep "$SECONDS_TO_RUN" \
    2>&1 | tee -a "$OUTPUT"
else
  sudo perf stat -p "$CHILD_PID" \
    -e "$EVENTS" \
    -- sleep "$SECONDS_TO_RUN" \
    2>&1 | tee -a "$OUTPUT"
fi

if [[ -n "${CHILD_PID:-}" ]] && kill -0 "$CHILD_PID" 2>/dev/null; then
  kill "$CHILD_PID" 2>/dev/null || true
  wait "$CHILD_PID" 2>/dev/null || true
fi

{
  echo
  echo "# app output"
  cat "$APP_OUTPUT" 2>/dev/null || true
} | tee -a "$OUTPUT"

rm -f "$APP_OUTPUT"
trap - EXIT INT TERM