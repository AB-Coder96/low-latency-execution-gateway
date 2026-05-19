# SPSC Queue Perf Report

Label: `wsl`

## Input

| Field | Value |
|---|---|
| input_path | `reports/perf/wsl-spsc-queue-perf-final.txt` |
| date_utc | 2026-05-19T07:01:52Z |
| host | ArazDesktop |
| kernel | Linux ArazDesktop 6.6.114.1-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Mon Dec  1 20:46:23 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux |
| command | `./build/debug/apps/fgep_spsc_queue_perf --payload-size 64` |

## Queue run summary

| Metric | Value |
|---|---:|
| payload_size_bytes | 64 |
| produced | 48124000 |
| consumed | 48124000 |
| submit_queued | 48124000 |
| submit_rejected | 175505 |
| ring_full | 175505 |
| elapsed_seconds | 10.002622 |
| throughput_handoffs_per_sec | 4811138.34 |
| throughput_mhandoffs_per_sec | 4.811 |

## Perf counters

| Counter | Value |
|---|---:|
| cycles | 44247098013 |
| instructions | 57627773276 |
| branches | 9196628839 |
| branch_misses | 21169764 |
| cache_references | 779317669 |
| cache_misses | 1712832 |
| l1_dcache_loads | 17911613387 |
| l1_dcache_load_misses | 606028458 |
| dtlb_loads | 17911261343 |
| dtlb_load_misses | 155429 |

## Derived metrics

| Metric | Value |
|---|---:|
| ipc_instructions_per_cycle | 1.302 |
| cycles_per_handoff | 919.44 |
| instructions_per_handoff | 1197.49 |
| branch_miss_rate_percent | 0.2302 |
| l1d_miss_rate_percent | 3.3834 |
| dtlb_miss_rate_percent | 0.000868 |
| ring_full_rate_percent | 0.3634 |

## System metadata

| Field | Value |
|---|---|
| generated_at_local | 2026-05-19 03:02:04 |
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
| git_commit | ba7d3f595f97 |
| have_dpdk | false |
| have_afxdp | false |
