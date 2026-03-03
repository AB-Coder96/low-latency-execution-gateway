#!/usr/bin/env bash
set -euo pipefail

echo "date=$(date -Iseconds)"
echo "uname=$(uname -a)"
if [[ -f /proc/cpuinfo ]]; then
  echo "cpu_model=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2- | sed 's/^ //')"
fi
if [[ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]]; then
  echo "cpu_governor=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
else
  echo "cpu_governor=unknown"
fi
if command -v lscpu >/dev/null 2>&1; then
  echo "lscpu_summary=$(lscpu | tr '\n' ';' | sed 's/;*$//')"
fi
