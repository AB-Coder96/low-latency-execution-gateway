# SPSC Queue Perf Report

Label: `ec2-aws-ena-dpdk`

## Input

| Field | Value |
|---|---|
| input_path | `reports/perf/ec2-spsc-queue-perf.txt` |
| date_utc | 2026-05-19T04:19:48Z |
| host | ip-172-31-72-198 |
| kernel | Linux ip-172-31-72-198 7.0.0-1004-aws #4-Ubuntu SMP PREEMPT Mon Apr 13 13:14:24 UTC 2026 x86_64 GNU/Linux |
| command | `./build/debug/apps/fgep_spsc_queue_perf --payload-size 64` |

## Queue run summary

| Metric | Value |
|---|---:|
| payload_size_bytes | 0 |
| produced | 0 |
| consumed | 0 |
| submit_queued | 0 |
| submit_rejected | 0 |
| ring_full | 0 |
| elapsed_seconds | 10.046225 |
| throughput_handoffs_per_sec | 0.00 |
| throughput_mhandoffs_per_sec | 0.000 |

## Perf counters

| Counter | Value |
|---|---:|
| cycles | 54558188140 |
| instructions | 119765604993 |
| branches | 16489020475 |
| branch_misses | 39540461 |
| cache_references | 0 |
| cache_misses | 0 |
| l1_dcache_loads | 40965487700 |
| l1_dcache_load_misses | 8737997 |
| dtlb_loads | 40927833694 |
| dtlb_load_misses | 134407 |

## Derived metrics

| Metric | Value |
|---|---:|
| ipc_instructions_per_cycle | 2.195 |
| cycles_per_handoff | 0.00 |
| instructions_per_handoff | 0.00 |
| branch_miss_rate_percent | 0.2398 |
| l1d_miss_rate_percent | 0.0213 |
| dtlb_miss_rate_percent | 0.000328 |
| ring_full_rate_percent | 0.0000 |

## System metadata

| Field | Value |
|---|---|
| generated_at_local | 2026-05-19 05:28:20 |
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
