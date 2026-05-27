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
| recording | 100000 | 100000 | 0 | 17988385 ns | 5559142 candidates/s | 51 | 1264 | 19485 |
| kernel_udp | 100000 | 100000 | 0 | 379957333 ns | 263187 candidates/s | 3522 | 8885 | 59872 |
| afxdp | 100000 | 0 | 100000 | 4199807 ns | 23810617 candidates/s | 20 | 29 | 41 |
| dpdk | 100000 | 0 | 100000 | 4083201 ns | 24490589 candidates/s | 20 | 27 | 33 |

## System metadata

| Field | Value |
|---|---|
| generated_at_local | 2026-05-27 09:05:59 |
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
