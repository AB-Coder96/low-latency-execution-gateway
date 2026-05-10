#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   ./scripts/build.sh
#   ./scripts/build.sh debug
#   ./scripts/build.sh release
#   ./scripts/build.sh bench
#
# Presets are defined in CMakePresets.json.

PRESET="${1:-debug}"

case "${PRESET}" in
    debug|release|bench)
        ;;
    *)
        echo "Unknown build preset: ${PRESET}"
        echo "Valid presets: debug, release, bench"
        exit 1
        ;;
esac

echo "Configuring preset: ${PRESET}"
cmake --preset "${PRESET}"

echo "Building preset: ${PRESET}"
cmake --build --preset "${PRESET}"

echo "Build complete."