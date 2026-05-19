# SPSC Queue Perf Report

Label: `ec2-aws-ena-dpdk`

## Input

| Field | Value |
|---|---|
| input_path | `reports/perf/ec2-spsc-queue-perf-final.txt` |
| date_utc | 2026-05-19T06:15:09Z |
| host | ip-172-31-72-198 |
| kernel | Linux ip-172-31-72-198 7.0.0-1004-aws #4-Ubuntu SMP PREEMPT Mon Apr 13 13:14:24 UTC 2026 x86_64 GNU/Linux |
| command | `./build/debug/apps/fgep_spsc_queue_perf --payload-size 64` |

## Queue run summary

| Metric | Value |
|---|---:|
| payload_size_bytes | 64 |
| produced | 109429103 |
| consumed | 109429103 |
| submit_queued | 109429103 |
| submit_rejected | 352037 |
| ring_full | 352037 |
| elapsed_seconds | 10.007982 |
| throughput_handoffs_per_sec | 10934182.93 |
| throughput_mhandoffs_per_sec | 10.934 |

## Perf counters

| Counter | Value |
|---|---:|
| cycles | 57108485647 |
| instructions | 125821747367 |
| branches | 17287273111 |
| branch_misses | 44233701 |
| cache_references | 0 |
| cache_misses | 0 |
| l1_dcache_loads | 43044140101 |
| l1_dcache_load_misses | 6801974 |
| dtlb_loads | 43020045111 |
| dtlb_load_misses | 120090 |

## Derived metrics

| Metric | Value |
|---|---:|
| ipc_instructions_per_cycle | 2.203 |
| cycles_per_handoff | 521.88 |
| instructions_per_handoff | 1149.80 |
| branch_miss_rate_percent | 0.2559 |
| l1d_miss_rate_percent | 0.0158 |
| dtlb_miss_rate_percent | 0.000279 |
| ring_full_rate_percent | 0.3207 |

## System metadata

| Field | Value |
|---|---|
| generated_at_local | 2026-05-19 06:15:21 |
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
