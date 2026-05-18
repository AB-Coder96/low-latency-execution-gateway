# Execution Benchmark

Backend: `simulated-execution`

Deterministic synthetic timestamps; not wall-clock performance.

## Summary

| Metric | Value |
|---|---:|
| candidate_count | 10000 |
| accepted_count | 9982 |
| rejected_count | 18 |
| live_order_count | 9982 |
| simulated_elapsed | 9999130 ns |
| throughput | 1000087 candidates/s |

## End-to-end latency

| Metric | Value |
|---|---:|
| count | 10000 |
| min_ns | 130 |
| max_ns | 130 |
| mean_ns | 130 |
| p50_ns | 130 |
| p90_ns | 130 |
| p99_ns | 130 |
| p999_ns | 130 |

## System metadata

| Field | Value |
|---|---|
| generated_at_local | 2026-05-17 23:22:52 |
| hostname | ArazDesktop |
| os_name | Ubuntu 24.04.4 LTS |
| kernel_name | Linux |
| kernel_release | 6.6.114.1-microsoft-standard-WSL2 |
| kernel_version | #1 SMP PREEMPT_DYNAMIC Mon Dec  1 20:46:23 UTC 2025 |
| machine | x86_64 |
| cpu_model | 11th Gen Intel(R) Core(TM) i7-1165G7 @ 2.80GHz |
| compiler | gcc 13.3.0 |
| build_type | Debug |
| default_backend | kernel_udp |
| git_commit | 31dd912c1c95 |
| have_dpdk | false |
| have_afxdp | false |
