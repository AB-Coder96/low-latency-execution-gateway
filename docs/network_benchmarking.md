# Network Benchmarking

This document describes the intended network benchmark policy for FGEP.

Network benchmarking should be explicit about what is measured and what is not measured.

## Measurement categories

FGEP reports should distinguish:

```text
synthetic deterministic latency
wall-clock host latency
network end-to-end latency
```

Synthetic deterministic latency is useful for repeatable regression tests.

Wall-clock host latency is useful for local software measurements, but includes clock overhead and host noise.

Network end-to-end latency should be reserved for actual sender/receiver runtime measurements.

## Localhost benchmarks

Localhost benchmarks are useful for testing:

```text
UDP packet construction
UDP receive path
MoldUDP64 decode path
sequence checking
runtime allocation behavior
report formatting
```

Localhost benchmarks do not prove real network performance.

They bypass physical NIC behavior and switch behavior.

## EC2 benchmarks

EC2 benchmarks can be useful for reproducible cloud experiments.

They measure software runtime under AWS virtualized networking.

They are not a claim of:

```text
colocated exchange latency
FPGA hardware latency
deterministic market-access latency
```

Reports should state this caveat clearly.

## Recommended EC2 setup

For two-EC2 tests, prefer:

```text
same AWS region
same availability zone
same VPC/subnet when possible
cluster placement group when possible
security group allowing UDP from generator to receiver
same AMI/kernel family
captured instance type
captured CPU model
captured kernel version
captured network interface name
```

## Generator/receiver model

Future network benchmark apps should use this shape:

```text
traffic generator
    -> UDP packets containing MoldUDP64 payloads
    -> network
    -> receiver
    -> decode
    -> sequence checks
    -> optional book/runtime processing
    -> report
```

Generator metrics:

```text
packets sent
messages sent
bytes sent
send errors
configured rate
actual send duration
seed/configuration
```

Receiver metrics:

```text
packets received
bytes received
messages decoded
decode errors
sequence gaps
out-of-order packets
throughput
p50/p99/p999 receive-to-process latency
runtime allocation status
page faults if available
hardware counters if available
```

## Clock policy

Wall-clock measurements should document the clock source.

Per-message timing can be expensive because it may call the host clock repeatedly.

If per-submit or per-packet timing is enabled, reports should mention clock-read overhead.

For high-rate benchmarks, consider:

```text
sampling
batch timing
hardware counters
external packet timestamping if available
```

## Runtime allocation policy

Network receiver hot paths should eventually use:

```text
caller-provided packet buffers
fixed decode output buffers
fixed queues
fixed telemetry recorders
preallocated runtime state
```

The receiver should report whether no-heap-after-initialization checks were enabled and passed.

## Linux I/O policy

Linux kernel networking should start with:

```text
nonblocking UDP sockets
epoll
eventfd shutdown wakeup
timerfd telemetry ticks
caller-provided buffers
```

Later alternatives may include AF_XDP or DPDK paths, but those should be measured separately and documented clearly.

## Report metadata

Network reports should capture best-effort metadata:

```text
hostname
kernel version
CPU model
core count
NUMA node count if available
network interface name
private IP when safe
build preset
git commit
runtime options
```

Metadata collection should not cause the benchmark to fail if a field is unavailable.

## Benchmark interpretation

A valid network report should answer:

```text
What binary ran?
What build preset was used?
What host and kernel ran it?
What traffic rate was requested?
How many packets/messages were sent?
How many packets/messages were received?
Were there gaps or decode errors?
What latency type is reported?
What limitations apply?
```

Avoid vague claims like:

```text
ultra-low latency
exchange-grade latency
FPGA latency
production-ready network latency
```

unless the measurement actually supports them.