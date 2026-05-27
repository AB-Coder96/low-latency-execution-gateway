# Wall Clock Backend Benchmark

Backend: `recording/kernel_udp/afxdp/dpdk`

Measured with std::chrono::steady_clock on the host. Per-submit latency includes host clock-read overhead. Kernel UDP performs real sendto(); AF_XDP/DPDK may reject if unsupported or unavailable.

## Measurement notes

| Field | Value |
|---|---|
| latency_type | wall-clock host latency |
| warmup_count | 0 |
| notes | Wall-clock samples are measured with the host steady clock around the benchmarked operation. They include host clock-read overhead, scheduler effects, and local system noise. |

## Wall-clock backend comparison

| Technology | Submissions | Accepted | Rejected | Elapsed | Throughput | p50_ns | p99_ns | p999_ns |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| recording | 100000 | 100000 | 0 | 11149253 ns | 8969210 candidates/s | 31 | 829 | 11890 |
| kernel_udp | 100000 | 100000 | 0 | 212880902 ns | 469746 candidates/s | 1998 | 3524 | 12952 |
| afxdp | 100000 | 0 | 100000 | 3240828 ns | 30856312 candidates/s | 16 | 19 | 22 |
| dpdk | 100000 | 0 | 100000 | 3273608 ns | 30547334 candidates/s | 16 | 21 | 32 |

## System metadata

| Field | Value |
|---|---|
| generated_at_local | 2026-05-27 08:58:09 |
| hostname | ArazDesktop |
| os_name | Ubuntu 24.04.4 LTS |
| kernel_name | Linux |
| kernel_release | 6.6.114.1-microsoft-standard-WSL2 |
| kernel_version | #1 SMP PREEMPT_DYNAMIC Mon Dec  1 20:46:23 UTC 2025 |
| machine | x86_64 |
| cpu_model | 11th Gen Intel(R) Core(TM) i7-1165G7 @ 2.80GHz |
| compiler | gcc 13.3.0 |
| build_type | Release |
| default_backend | kernel_udp |
| git_commit | a83fa160a263 |
| have_dpdk | false |
| have_afxdp | false |
