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
