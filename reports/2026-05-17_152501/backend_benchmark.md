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
