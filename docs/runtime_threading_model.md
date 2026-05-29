# Runtime Threading Model

This document describes the intended threading model for FGEP runtime components.

The main rule is:

```text
Do not make every class internally thread-safe.
Use single-owner mutable state and queues between owners.
```

## Why single-owner state

Low-latency runtime code is easier to reason about when each mutable object has one owner.

A single-owner model avoids:

```text
coarse mutexes
random shared access
accidental contention
unclear memory ordering
data races hidden behind APIs
```

The owner thread mutates the object directly. Other threads communicate with that owner by sending commands or data through queues.

## Initial runtime thread model

The first threaded runtime should be simple:

```text
Thread A: book/replay thread
Thread B: order/risk/lifecycle/backend thread
Thread C: control/telemetry thread
```

Later versions may split this further:

```text
Thread 1: market data RX/decode
Thread 2: ITCH apply/book update
Thread 3: order candidate/risk/lifecycle
Thread 4: execution backend TX
Thread 5: control/telemetry
```

## Mutable state ownership

Recommended ownership:

| State | Owner |
|---|---|
| market-data socket RX buffers | market-data RX thread |
| MoldUDP64/ITCH decode buffers | decode thread |
| venue books | book thread |
| order candidate generator | order thread |
| risk mutable state | order/risk thread |
| lifecycle engine | order/lifecycle thread |
| backend submit queue producer side | order/lifecycle thread |
| backend submit queue consumer side | backend TX thread |
| telemetry counters | owning runtime components, scraped by telemetry |

## Queues

Queues are the preferred cross-thread communication mechanism.

Use queues for:

```text
market-data packets
decoded message batches
order candidates
backend submit requests
control-plane updates
telemetry events
shutdown notifications
```

Queue rules:

```text
fixed capacity
no heap allocation in push/pop
deterministic full behavior
payload ownership is clear
producer and consumer indexes are cache separated
acquire/release ordering is documented
```

## Atomics

Atomics are allowed for simple cross-thread state, but they should be used deliberately.

Good atomic use cases:

```text
shutdown flag
runtime phase flag
single producer/consumer queue indexes
cache-line-separated counters
published immutable snapshot pointer/index
```

Bad atomic use cases:

```text
making a complex mutable book "thread-safe"
publishing partially updated state
mixing atomics and non-atomics without ownership
using relaxed ordering for data publication
```

Simple counters may use:

```text
std::memory_order_relaxed
```

Queue publication should use acquire/release ordering.

Snapshot publication should use:

```text
publish: release
read: acquire
```

## Books and lifecycle state

Books and lifecycle engines should not be randomly shared across threads.

Avoid this pattern:

```text
Thread A mutates book
Thread B reads book internals
Thread C cancels orders directly
Thread D modifies lifecycle state
```

Prefer this pattern:

```text
Book thread owns books.
Other threads send book commands or consume published summaries.

Lifecycle thread owns live orders.
Other threads send order commands or consume published outcomes.
```

## Control-plane updates

Control-plane updates should not mutate hot-path state from arbitrary threads.

Preferred model:

```text
control thread receives update
control thread builds immutable snapshot
snapshot is published with release ordering
runtime owner thread loads snapshot with acquire ordering
runtime owner applies snapshot at a safe boundary
```

Early versions may use `std::shared_ptr<const Snapshot>`.

Later no-heap runtime versions should use fixed double-buffered or ring-buffered snapshot storage.

## Telemetry

Telemetry should avoid blocking the hot path.

Allowed telemetry patterns:

```text
relaxed atomic counters
fixed telemetry queues
fixed-capacity latency recorders
periodic scrape by telemetry thread
```

Avoid:

```text
allocating strings on every event
writing files in hot path
taking locks for every counter update
formatting Markdown during runtime
```

## Shutdown

Shutdown should be explicit and joinable.

Preferred sequence:

```text
publish shutdown request
wake blocked runtime threads
stop accepting new work
drain bounded queues if required
join worker threads
write final reports
```

Runtime destructors should not silently leave worker threads running.

## Testing policy

Threaded runtime tests should cover:

```text
start
stop
join
queue handoff
bounded overflow
shutdown while idle
shutdown while work is queued
ThreadSanitizer cleanliness when enabled
```

Threaded tests should be enabled only when thread support is configured.

```text
FGEP_ENABLE_THREADS=ON
```