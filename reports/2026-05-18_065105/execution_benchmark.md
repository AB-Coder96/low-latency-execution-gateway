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
| generated_at_local | 2026-05-18 06:51:05 |
| hostname | ip-172-31-76-42 |
| os_name | Ubuntu 26.04 LTS |
| kernel_name | Linux |
| kernel_release | 7.0.0-1004-aws |
| kernel_version | #4-Ubuntu SMP PREEMPT Mon Apr 13 13:14:24 UTC 2026 |
| machine | x86_64 |
| cpu_model | Intel(R) Xeon(R) Platinum 8488C |
| compiler | gcc 15.2.0 |
| build_type | Debug |
| default_backend | kernel_udp |
| git_commit | c5767d301f51 |
| have_dpdk | false |
| have_afxdp | false |
