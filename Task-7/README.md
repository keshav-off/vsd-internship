# Timer IP — VSDSquadron FPGA

Commercial-grade, plug-and-play programmable countdown Timer IP for the
VSDSquadron RISC-V SoC. Memory-mapped, 32-bit, one-shot or periodic
(auto-reload) countdown with an optional prescaler and a
write-1-to-clear status flag.

## What is this?

A hardware countdown timer you can drop into any VSDSquadron RISC-V SoC
via 4 memory-mapped registers (`CTRL`, `LOAD`, `VALUE`, `STATUS`). Use it
for delays, periodic software events, or a polled elapsed-time source —
no interrupt controller required.

## Repository Structure

```
timer_ip/
├── rtl/
│   ├── timer_ip.v                     # The IP core (drop-in module)
│   └── riscv_integration_snippet.v    # Copy-paste top-level wiring reference
├── software/
│   ├── Timer_test.c                   # One-shot mode example
│   ├── Timer_periodic_test.c          # Periodic / auto-reload example
│   └── Timer_clear_test.c             # STATUS write-1-to-clear example
├── docs/
│   ├── IP_User_Guide.md               # Overview, features, block diagram, programming model
│   ├── Register_Map.md                # Full bit-level register datasheet
│   ├── Integration_Guide.md           # How to wire this into your SoC top-level
│   └── Example_Usage.md               # Board usage, validation, expected output, limitations
└── README.md                          # You are here
```

## How to Integrate It

1. Copy `rtl/timer_ip.v` into your project's RTL sources.
2. Instantiate it in your SoC top-level and wire up address decode —
   full copy-paste block in `rtl/riscv_integration_snippet.v`.
3. Full walkthrough: **[`docs/Integration_Guide.md`](docs/Integration_Guide.md)**.

## Where to Find the Docs

| Document | Contents |
|---|---|
| [`docs/IP_User_Guide.md`](docs/IP_User_Guide.md) | What it is, feature list, limitations, block diagram, programming model |
| [`docs/Register_Map.md`](docs/Register_Map.md) | Register offsets, bit fields, reset values, R/W behavior |
| [`docs/Integration_Guide.md`](docs/Integration_Guide.md) | Step-by-step SoC integration, address decode, signal list |
| [`docs/Example_Usage.md`](docs/Example_Usage.md) | Board-level usage, example firmware walkthrough, expected output, common failure symptoms |

## How to Test It

Flash any of the three example programs in `software/` to your
VSDSquadron board and watch the UART output:

```c
IO_OUT(IO_TIMER_LOAD, 100000);           // load a countdown value
IO_OUT(IO_TIMER_CTRL, 0x1);              // enable, one-shot mode
while (!(IO_IN(IO_TIMER_STATUS) & 0x1)); // poll for timeout
printf("TIMEOUT OCCURRED!\n");
```

Expected: the program prints the loaded value, a few decreasing `VALUE`
reads, then `TIMEOUT OCCURRED!` once the counter reaches zero. Full
expected output and failure-symptom table: `docs/Example_Usage.md`.

## Key Feature Summary

- 32-bit countdown, one-shot or periodic (auto-reload) mode
- Optional 8-bit prescaler
- Write-1-to-clear `STATUS` flag (poll-based, no interrupt line)
- Single channel per instance — instantiate multiple copies for more timers

## License

Released under the Apache License 2.0, consistent with other VSD community IP.
