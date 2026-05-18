# EC2 DPDK Setup

This document records the EC2 setup used to make the project detect DPDK and prepare for kernel UDP vs DPDK backend benchmarking.

## Safety rule

Never bind the primary SSH/default-route ENI to DPDK/VFIO.

Use a secondary ENI for DPDK experiments. If the primary ENI is rebound away from the Linux kernel driver, the SSH session can be lost.

## Target

The immediate target of this setup is:

```text
| have_dpdk | true |