# CI

This repo uses GitHub Actions:

- **CI**: build + unit tests on push/PR
- **Nightly perf baseline**: scheduled run that produces CSV artifacts

## CI workflow

- uses CMake `Release`
- runs GoogleTest suite

## Nightly perf baseline

The nightly job runs `scripts/ci_nightly_perf.sh` and uploads CSVs as artifacts.
This does **not** guarantee stable performance across GitHub-hosted runners, but it gives:

- a consistent smoke-test for regressions
- a convenient CSV history for local comparison
