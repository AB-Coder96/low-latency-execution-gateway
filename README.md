# FPGA-Gated Execution Platform

C++20 byte-accurate ITCH/OUCH replay, order book, risk, and execution platform with a future Corundum FPGA/NIC permissive gate.

## Goal

Build a CPU-first low-latency execution platform, then add a narrow Corundum FPGA/NIC guardrail that enforces slow-risk state inline without requiring CPU approval for every order.

The project is intentionally staged:

1. Byte-accurate protocol foundations.
2. Replay and multi-symbol/multi-venue book state.
3. Normalized BBO.
4. OUCH-style order lifecycle.
5. Software risk supervisor.
6. Kernel UDP baseline and AF_XDP fast path.
7. Corundum host-to-hardware guardrail updates.
8. Inline FPGA/NIC permissive gate.
9. Benchmarking and acceptance reports.

## Locked scope

### Data side

- Nasdaq TotalView-ITCH-style replay.
- MoldUDP64 packet framing.
- SoupBinTCP session framing later.
- Two venue-local books.
- Multi-symbol support.
- One normalized best-bid/best-offer layer.

### Execution side

- Nasdaq OUCH 5.0-style order-entry semantics.
- New / cancel / replace / partial-fill / reject simulation.
- Order lifecycle engine.
- Software risk supervisor.
- Later execution backends:
  - Kernel UDP baseline.
  - AF_XDP optimized path.
  - DPDK is future-only, not MVP.

### Hardware extension

- Corundum-based host-to-hardware guardrail updates.
- Inline FPGA/NIC permissive gate.
- Hardware gate enforces:
  - global halt
  - symbol enable
  - cancel-only
  - max size
  - max notional

### Acceptance targets

- 250k order candidates/s end-to-end.
- 25% lower p99 submission latency versus kernel UDP baseline with optimized path.
- 30% lower jitter versus kernel UDP baseline with optimized path.
- CPU-to-hardware guardrail update p99 below 20 microseconds.
- 0 post-halt leaked orders across 1,000,000 synthetic order candidates.
- 12/12 risk/permissive tests passed.
- 100% event-sequence correctness across replay, risk, and execution flows.

## Architecture

Correct layering:

```txt
raw bytes
-> transport/session framing
-> exact ITCH/OUCH payload decode
-> decoded wire message structs
-> replay adapter
-> instrument directory
-> venue-local symbol books
-> normalized BBO
-> order lifecycle
-> risk supervisor
-> execution backend
-> telemetry
-> Corundum control path
-> FPGA/NIC permissive gate
```

Important rule:

```txt
Do not treat core/market_event.hpp or core/order_event.hpp as wire protocol specs.
```

Use protocol-specific modules for byte-level parsing:

```txt
include/fgep/wire/
include/fgep/itch/
include/fgep/ouch/
include/fgep/moldudp64/
include/fgep/soupbintcp/
```

Never parse protocol messages using packed structs or `reinterpret_cast`.

All wire parsing should be explicit by offset.

## Current major modules

### Core

```txt
include/fgep/core/types.hpp
include/fgep/core/time.hpp
include/fgep/core/errors.hpp
include/fgep/core/market_event.hpp
include/fgep/core/order_event.hpp
```

Core types are internal normalized domain vocabulary only.

### Wire helpers

```txt
include/fgep/wire/byte_io.hpp
include/fgep/wire/fixed_ascii.hpp
```

Expected helpers:

```txt
read_u8
read_u16_be
read_u32_be
read_u48_be
read_u64_be
write_u8
write_u16_be
write_u32_be
write_u48_be
write_u64_be
fixed-width ASCII read/write helpers
```

### ITCH

```txt
include/fgep/itch/itch_types.hpp
include/fgep/itch/itch_message_type.hpp
include/fgep/itch/itch_wire_messages.hpp
include/fgep/itch/itch_decode.hpp
include/fgep/itch/itch_encode.hpp

src/itch/itch_decode.cpp
src/itch/itch_encode.cpp
```

Implemented or intended ITCH byte-accurate messages:

```txt
S = System Event
R = Stock Directory
H = Stock Trading Action
A = Add Order - No MPID Attribution
F = Add Order with MPID Attribution
E = Order Executed
C = Order Executed with Price
X = Order Cancel
D = Order Delete
U = Order Replace
P = Trade Non-Cross
Q = Cross Trade
B = Broken Trade
```

Important ITCH protocol rules:

```txt
integer fields are big-endian
alpha fields are ASCII, left-justified, right-padded with spaces
timestamps are 6-byte integers: nanoseconds since midnight
common payload header:
    offset 0: message type, length 1
    offset 1: stock locate, length 2
    offset 3: tracking number, length 2
    offset 5: timestamp, length 6
body starts at offset 11
```

Important Cross Trade rule:

```txt
Q = Cross Trade uses an 8-byte shares field.
Do not narrow Cross Trade shares to uint32_t.
Use uint64_t / CrossShares.
```

### OUCH

```txt
include/fgep/ouch/ouch_types.hpp
include/fgep/ouch/ouch_message_type.hpp
include/fgep/ouch/ouch_wire_messages.hpp
include/fgep/ouch/ouch_decode.hpp
include/fgep/ouch/ouch_encode.hpp

src/ouch/ouch_decode.cpp
src/ouch/ouch_encode.cpp
```

Initial OUCH scope:

```txt
O = Enter Order
U = Replace Order Request
X = Cancel Order Request
```

Important OUCH rules:

```txt
integer fields are big-endian
prices have 4 implied decimal places
timestamps are nanoseconds since midnight where present
UserRefNum is the main order transaction identifier
UserRefNum must be unique and strictly increasing for a day/account/port
payload parsing must be explicit by offset
never use reinterpret_cast
```

Cancel Order Request may be 9 bytes without appendage length, or 11+ bytes with appendage length and optional appendage, depending on how the current code implemented optional appendage support.

### MoldUDP64

```txt
include/fgep/moldudp64/moldudp64_types.hpp
include/fgep/moldudp64/moldudp64_packet.hpp
include/fgep/moldudp64/moldudp64_decode.hpp
include/fgep/moldudp64/moldudp64_encode.hpp

src/moldudp64/moldudp64_decode.cpp
src/moldudp64/moldudp64_encode.cpp
```

MoldUDP64 downstream packet shape:

```txt
Session:          offset 0,  length 10
Sequence Number:  offset 10, length 8
Message Count:    offset 18, length 2
```

Message block:

```txt
Message Length: 2 bytes
Message Data:   Message Length bytes
```

Special message counts:

```txt
0      = heartbeat
0xFFFF = end of session
```

Important current design decision:

```txt
MoldUDP64 Session is raw 10 bytes:
using Session = std::array<std::byte, 10>;
```

Do not use ITCH fixed ASCII helpers to compare MoldUDP64 sessions.

Use MoldUDP64-specific helpers such as:

```cpp
session_equals(packet.session, "SESSION001");
make_session_from_text("SESSION001");
```

Known issue to check:

```txt
tests/test_moldudp64_decode.cpp may still use wire::fixed_ascii_equals(packet.session, ...)
That must be replaced with session_equals(packet.session, ...)
```

### Replay

```txt
include/fgep/replay/moldudp64_itch_replay.hpp
src/replay/moldudp64_itch_replay.cpp
```

Purpose:

```txt
MoldUDP64 packet bytes
-> decoded MoldUDP64 message blocks
-> ITCH payload decode
-> sequence-numbered decoded ITCH messages
```

Planned/current next replay layer:

```txt
include/fgep/replay/itch_replay_apply.hpp
src/replay/itch_replay_apply.cpp
```

Purpose:

```txt
decoded ITCH replay messages
-> R/H update InstrumentDirectory
-> A/F/E/C/X/D/U update MultiVenueBook
```

### Venue and instrument identity

```txt
include/fgep/venue/mic.hpp
include/fgep/venue/venue.hpp
include/fgep/instrument/instrument_key.hpp
include/fgep/instrument/instrument_directory.hpp

src/instrument/instrument_directory.cpp
```

Design:

```txt
VenueId + StockLocate = hot-path instrument key
StockSymbol           = canonical symbol for normalized BBO
MIC                   = official ISO venue identifier, used as metadata/config
```

Important distinction:

```txt
StockLocate is venue/session-local.
Do not use StockLocate alone for normalized cross-venue BBO.
Use CanonicalSymbol for normalized BBO lookup.
```

### Book and BBO

```txt
include/fgep/book/book_types.hpp
include/fgep/book/symbol_order_book.hpp
include/fgep/book/venue_book.hpp
include/fgep/book/multi_venue_book.hpp
include/fgep/bbo/normalized_bbo.hpp

src/book/symbol_order_book.cpp
src/book/venue_book.cpp
src/book/multi_venue_book.cpp
src/bbo/normalized_bbo.cpp
```

Book layering:

```txt
MultiVenueBook
-> VenueBook
   -> SymbolOrderBook
      -> resting order map
      -> bid levels
      -> ask levels
```

BBO rule:

```txt
best bid = highest bid price across venues
best ask = lowest ask price across venues
tie-break 1 = larger displayed quantity
tie-break 2 = lower VenueId for deterministic output
```

## Build

Typical build:

```bash
./scripts/build.sh
```

Typical test:

```bash
./scripts/test.sh
```

If CMake fails early, check:

```txt
The root CMake must include:
cmake/CompileWarnings.cmake

not:
cmake/CompilerWarnings.cmake
```

The main library should be a real static library, not only an INTERFACE library, once `.cpp` files are added.

Example source list should include whichever files exist:

```cmake
add_library(
    fgep
    STATIC
        src/core/time.cpp
        src/core/types.cpp
        src/itch/itch_decode.cpp
        src/itch/itch_encode.cpp
        src/ouch/ouch_decode.cpp
        src/ouch/ouch_encode.cpp
        src/moldudp64/moldudp64_decode.cpp
        src/moldudp64/moldudp64_encode.cpp
        src/replay/moldudp64_itch_replay.cpp
        src/replay/itch_replay_apply.cpp
        src/instrument/instrument_directory.cpp
        src/book/symbol_order_book.cpp
        src/book/venue_book.cpp
        src/book/multi_venue_book.cpp
        src/bbo/normalized_bbo.cpp
)
```

## Test coverage currently expected

Representative tests:

```txt
test_core_types.cpp
test_errors.cpp
test_fixed_ascii.cpp
test_wire_byte_io.cpp

test_itch_message_type.cpp
test_itch_decode_encode.cpp
test_itch_stock_directory_decode_encode.cpp
test_itch_stock_trading_action_decode_encode.cpp
test_itch_instrument_directory_integration.cpp

test_ouch_decode_encode.cpp

test_moldudp64_decode.cpp
test_moldudp64_decode_encode.cpp
test_moldudp64_itch_integration.cpp
test_moldudp64_itch_replay.cpp

test_instrument_directory.cpp
test_multi_venue_book_and_bbo.cpp
test_replay_to_books_integration.cpp
```

If a test exists but the source module does not, either add the source module or remove the test from `tests/CMakeLists.txt`.

## Reference data plan

Use CSV for ingesting official venue reference data.

Recommended layout:

```txt
data/reference/mic/ISO10383_MIC.csv
data/reference/symbols/nasdaqlisted.txt
data/reference/symbols/otherlisted.txt

data/fixtures/venues/venues_2_nasdaq_family.csv
data/fixtures/symbols/symbols_8.csv
data/fixtures/symbols/symbols_128.csv
data/fixtures/symbols/symbols_512.csv
```

Benchmark symbol profiles:

```txt
Correctness tests:     8 symbols × 2 venues
Default benchmark:   128 symbols × 2 venues
Stress benchmark:    512 symbols × 2 venues
```

Event distribution:

```txt
128-symbol default:
    top 16 symbols receive 80% of events
    remaining 112 symbols receive 20% of events

512-symbol stress:
    top 32 symbols receive 80% of events
    remaining 480 symbols receive 20% of events
```

## Non-goals

```txt
No production broker-grade smart order router.
No dark pool routing.
No slicing engine.
No client best-execution compliance engine.
No attempt to move all strategy logic into hardware.
No frontend-first build.
No DPDK in MVP.
```

## Current next step

Before adding new features, make sure the current tree builds.

Known thing to check first:

```txt
MoldUDP64 Session is raw bytes.
Patch any old test that compares packet.session using wire::fixed_ascii_equals.
Use session_equals instead.
```

Then continue with:

```txt
replay -> instrument directory -> multi-venue book integration
```

After that, start:

```txt
OUCH order lifecycle engine
```
