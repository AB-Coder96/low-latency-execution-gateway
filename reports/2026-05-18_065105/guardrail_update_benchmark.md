# Guardrail Update Benchmark

Backend: `simulated-control-path`

These are deterministic simulated update-latency numbers.

## Summary

| Metric | Value |
|---|---:|
| update_count | 10000 |
| applied_count | 10000 |
| dropped_count | 0 |
| simulated_elapsed_ns | 10749850 |

## Update latency

| Metric | Value |
|---|---:|
| count | 10000 |
| min_ns | 1000 |
| max_ns | 1150 |
| mean_ns | 1074 |
| p50_ns | 1075 |
| p90_ns | 1150 |
| p99_ns | 1150 |
| p999_ns | 1150 |

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
