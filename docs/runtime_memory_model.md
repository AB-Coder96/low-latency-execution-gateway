# Runtime Memory Model

This document describes the intended memory model for FGEP runtime work.

The goal is to make allocation behavior explicit before replacing more runtime containers with fixed-capacity structures.

## Runtime phases

FGEP runtime code should be designed around explicit phases:

```text
initialization
    -> warmup
    -> running
    -> stopped
```

## Initialization phase

The initialization phase may allocate memory.

Allowed initialization work includes:

```text
load configuration
create execution backends
create queues
create fixed-capacity tables
reserve benchmark/report buffers
build instrument directories
build venue/book state
open sockets
create worker objects
```

This phase is allowed to use standard containers where they are not part of the selected runtime hot path.

## Warmup phase

The warmup phase prepares runtime structures so the first live operation does not pay setup costs.

Warmup may include:

```text
touching memory pages
priming queues
creating placeholder entries
running synthetic dry-run operations
initializing telemetry state
checking backend readiness
```

Warmup should not be used to hide unbounded runtime allocation. It should make fixed-capacity runtime state ready.

## Running phase

The running phase is the hot path.

Selected hot-path scenarios should eventually avoid heap allocation after initialization and warmup.

Runtime hot paths should prefer:

```text
fixed arrays
fixed-capacity queues
fixed open-addressing tables
direct-index tables
inline payload buffers
caller-provided decode buffers
preallocated telemetry storage
```

Runtime hot paths should avoid:

```text
std::vector growth
std::unordered_map insertion
std::map node allocation
std::string construction
shared mutable ownership
unbounded logging
filesystem output
dynamic polymorphic allocation
```

## Stopped phase

The stopped phase releases resources and writes final reports.

Allocation is allowed after runtime has stopped because this phase is not latency-sensitive.

Allowed stopped-phase work includes:

```text
joining threads
closing sockets
formatting reports
writing Markdown files
writing profiling outputs
destroying runtime objects
```

## No-heap-after-initialization goal

The project goal is not to ban all heap allocation everywhere.

The goal is narrower:

```text
For selected runtime hot-path scenarios, no heap allocation should occur after initialization and warmup.
```

Examples of selected hot paths:

```text
encoded backend submit
backend queue push/pop
order lifecycle enter/replace/cancel
book add/execute/cancel/delete/replace
fixed-buffer packet decode
runtime telemetry record
```

Reports, CLI tools, tests, setup code, and documentation tools may allocate.

## Ownership policy

Runtime state should have one clear owner.

Preferred ownership:

```text
one thread owns one mutable state object
other threads communicate with that owner using queues
configuration updates use immutable snapshots
telemetry uses counters or queues
```

Avoid randomly sharing these objects across threads:

```text
SymbolOrderBook
VenueBook
OrderLifecycleEngine
RiskSupervisor mutable state
GuardrailState mutable state
ExecutionBackend implementation state
```

If state must cross a thread boundary, prefer passing messages, snapshots, or fixed-size queue entries.

## Container policy

Current baseline containers are acceptable for correctness and reports.

Runtime variants should gradually replace allocation-heavy containers.

Examples:

| Current baseline | Runtime direction |
|---|---|
| `std::vector` samples | fixed-capacity latency recorder |
| `std::unordered_map` live orders | fixed live order table |
| `std::unordered_map` instruments | direct-index instrument directory |
| `std::map` price levels | fixed price-level arrays |
| vector-return decode APIs | decode-into caller buffer APIs |

## Allocation testing policy

No-heap tests should be opt-in.

They should run only when configured with:

```text
FGEP_ENABLE_RUNTIME_NO_HEAP_TESTS=ON
```

Test pattern:

```text
initialize component
warm up component
forbid heap allocations
run selected hot-path scenario
allow heap allocations
verify result
```

The guard should be used carefully because overriding global allocation affects the whole process.

## Reporting policy

Benchmark and report generation may allocate.

Runtime measurements should clearly label whether they are:

```text
synthetic deterministic latency
wall-clock host latency
future network end-to-end latency
```

Runtime memory claims should identify the exact scenario tested.