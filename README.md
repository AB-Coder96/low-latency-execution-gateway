# FPGA-Gated Execution Platform

C++20 ITCH/OUCH replay, order book, risk, and execution platform with kernel UDP, AF_XDP, and Corundum-style FPGA guardrail gating.

## Goal

This project is a hardware-adjacent low-latency execution platform built for deep learning and systems demonstration.

It models the full path:

```txt
ITCH-style replay
-> venue-local order books
-> normalized BBO
-> OUCH-style order lifecycle
-> software risk supervisor
-> execution backend
-> simulated / FPGA-style hardware gate
```

## Future Implementation
 - Network connectivity
 - Exchange Certification
 - Dark-pool Routing
 - Smart order Routing
 - compliance workflow
 - Full strategy logic in FPGA