#!/usr/bin/env bash
set -euo pipefail
# Usage:
#   ./scripts/test.sh
#   ./scripts/test.sh debug
#   ./scripts/test.sh release
#
# Test presets are defined in CMakePresets.json.

PRESET="${1:-debug}"

case "${PRESET}" in
    debug|release)
        ;;
    *)
        echo "Unknown test preset: ${PRESET}"
        echo "Valid presets: debug, release"
        exit 1
        ;;
esac

echo "Running tests with preset: ${PRESET}"
ctest --preset "${PRESET}" --output-on-failure

echo "Tests complete."