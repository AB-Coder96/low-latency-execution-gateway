# Wall Clock Backend Benchmark

Backend: `recording/kernel_udp/afxdp/dpdk`

Measured with std::chrono::steady_clock on the host. Kernel UDP performs real sendto(); AF_XDP/DPDK may reject if unsupported or unavailable.

## Wall-clock backend comparison

| Technology | Submissions | Accepted | Rejected | Elapsed | Throughput | p50_ns | p99_ns | p999_ns |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| recording | 100000 | 100000 | 0 | 11581855 ns | 8634195 candidates/s | 38 | 1190 | 2276 |
| kernel_udp | 100000 | 100000 | 0 | 178971375 ns | 558748 candidates/s | 1679 | 3053 | 10145 |
| afxdp | 100000 | 0 | 100000 | 5578279 ns | 17926675 candidates/s | 27 | 32 | 43 |
| dpdk | 100000 | 0 | 100000 | 5659802 ns | 17668462 candidates/s | 27 | 37 | 47 |

## System metadata

| Field | Value |
|---|---|
| generated_at_local | 2026-05-18 16:29:36 |
| hostname | ip-172-31-72-198 |
| os_name | Ubuntu 26.04 LTS |
| kernel_name | Linux |
| kernel_release | 7.0.0-1004-aws |
| kernel_version | #4-Ubuntu SMP PREEMPT Mon Apr 13 13:14:24 UTC 2026 |
| machine | x86_64 |
| cpu_model | Intel(R) Xeon(R) Platinum 8488C |
| compiler | gcc 15.2.0 |
| build_type | Release |
| default_backend | kernel_udp |
| git_commit | 4eb94128a71e |
| have_dpdk | false |
| have_afxdp | false |
