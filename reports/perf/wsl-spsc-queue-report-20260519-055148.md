# SPSC Queue Perf Report

Label: `wsl`

## Input

| Field | Value |
|---|---|
| input_path | `reports/perf/wsl-spsc-queue-perf-20260519-055148.txt` |
| date_utc | 2026-05-19T05:51:48Z |
| host | ArazDesktop |
| kernel | Linux ArazDesktop 6.6.114.1-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Mon Dec  1 20:46:23 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux |
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
| elapsed_seconds | 0.000000 |
| throughput_handoffs_per_sec | 0.00 |
| throughput_mhandoffs_per_sec | 0.000 |

## Perf counters

| Counter | Value |
|---|---:|
| cycles | 0 |
| instructions | 0 |
| branches | 0 |
| branch_misses | 0 |
| cache_references | 0 |
| cache_misses | 0 |
| l1_dcache_loads | 0 |
| l1_dcache_load_misses | 0 |
| dtlb_loads | 0 |
| dtlb_load_misses | 0 |

## Derived metrics

| Metric | Value |
|---|---:|
| ipc_instructions_per_cycle | 0.000 |
| cycles_per_handoff | 0.00 |
| instructions_per_handoff | 0.00 |
| branch_miss_rate_percent | 0.0000 |
| l1d_miss_rate_percent | 0.0000 |
| dtlb_miss_rate_percent | 0.000000 |
| ring_full_rate_percent | 0.0000 |

## System metadata

| Field | Value |
|---|---|
| generated_at_local | 2026-05-19 01:52:17 |
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
| git_commit | d18e45d87baa |
| have_dpdk | false |
| have_afxdp | false |
