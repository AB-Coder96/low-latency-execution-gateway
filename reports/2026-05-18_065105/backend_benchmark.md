# Backend Comparison Benchmark

Backend: `recording/kernel_udp/afxdp/dpdk`

Deterministic submit-latency model. Kernel UDP performs real sendto(); AF_XDP/DPDK may be unsupported unless implemented and available.

## Backend comparison

| Technology | Submissions | Accepted | Rejected | Simulated elapsed | Throughput | p50_ns | p99_ns | p999_ns |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| recording | 10000 | 10000 | 0 | 11249875 ns | 888898 candidates/s | 1125 | 1250 | 1250 |
| kernel_udp | 10000 | 10000 | 0 | 11249875 ns | 888898 candidates/s | 1125 | 1250 | 1250 |
| afxdp | 10000 | 0 | 10000 | 11249875 ns | 888898 candidates/s | 1125 | 1250 | 1250 |
| dpdk | 10000 | 0 | 10000 | 11249875 ns | 888898 candidates/s | 1125 | 1250 | 1250 |

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
