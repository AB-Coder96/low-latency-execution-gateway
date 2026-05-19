# SPSC Queue Perf Report

Label: `ec2-aws-ena-dpdk`

## Input

| Field | Value |
|---|---|
| input_path | `reports/perf/ec2-spsc-queue-perf-fixed.txt` |
| date_utc | 2026-05-19T06:08:33Z |
| host | ip-172-31-72-198 |
| kernel | Linux ip-172-31-72-198 7.0.0-1004-aws #4-Ubuntu SMP PREEMPT Mon Apr 13 13:14:24 UTC 2026 x86_64 GNU/Linux |
| command | `./build/debug/apps/fgep_spsc_queue_perf --payload-size 64` |

## Queue run summary

| Metric | Value |
|---|---:|
| payload_size_bytes | 64 |
| produced | 111716534 |
| consumed | 111716534 |
| submit_queued | 111716534 |
| submit_rejected | 80738 |
| ring_full | 80738 |
| elapsed_seconds | 10.008459 |
| throughput_handoffs_per_sec | 11162210.98 |
| throughput_mhandoffs_per_sec | 11.162 |

## Perf counters

| Counter | Value |
|---|---:|
| cycles | 57182374958 |
| instructions | 125823531748 |
| branches | 17303328018 |
| branch_misses | 41691088 |
| cache_references | 0 |
| cache_misses | 0 |
| l1_dcache_loads | 43042118288 |
| l1_dcache_load_misses | 6919611 |
| dtlb_loads | 43014182471 |
| dtlb_load_misses | 141725 |

## Derived metrics

| Metric | Value |
|---|---:|
| ipc_instructions_per_cycle | 2.200 |
| cycles_per_handoff | 511.85 |
| instructions_per_handoff | 1126.27 |
| branch_miss_rate_percent | 0.2409 |
| l1d_miss_rate_percent | 0.0161 |
| dtlb_miss_rate_percent | 0.000329 |
| ring_full_rate_percent | 0.0722 |

## System metadata

| Field | Value |
|---|---|
| generated_at_local | 2026-05-19 06:09:06 |
| hostname | ip-172-31-72-198 |
| os_name | Ubuntu 26.04 LTS |
| kernel_name | Linux |
| kernel_release | 7.0.0-1004-aws |
| kernel_version | #4-Ubuntu SMP PREEMPT Mon Apr 13 13:14:24 UTC 2026 |
| machine | x86_64 |
| cpu_model | Intel(R) Xeon(R) Platinum 8488C |
| compiler | gcc 15.2.0 |
| build_type | Debug |
| default_backend | kernel_udp |
| git_commit | 458df584c88b |
| have_dpdk | true |
| have_afxdp | false |
