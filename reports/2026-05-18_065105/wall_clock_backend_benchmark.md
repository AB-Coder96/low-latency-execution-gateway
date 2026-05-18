# Wall Clock Backend Benchmark

Backend: `recording/kernel_udp/afxdp/dpdk`

Measured with std::chrono::steady_clock on the host. Kernel UDP performs real sendto(); AF_XDP/DPDK may reject if unsupported or unavailable.

## Wall-clock backend comparison

| Technology | Submissions | Accepted | Rejected | Elapsed | Throughput | p50_ns | p99_ns | p999_ns |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| recording | 100000 | 100000 | 0 | 31720480 ns | 3152537 candidates/s | 142 | 1522 | 3319 |
| kernel_udp | 100000 | 100000 | 0 | 201778787 ns | 495592 candidates/s | 1887 | 3243 | 10616 |
| afxdp | 100000 | 0 | 100000 | 10744422 ns | 9307154 candidates/s | 50 | 62 | 80 |
| dpdk | 100000 | 0 | 100000 | 10638669 ns | 9399672 candidates/s | 50 | 62 | 80 |

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
