# Timer IP — User Guide

## 1. IP Overview

This IP provides a programmable countdown timer suitable for delays,
periodic events, and software-controlled timing on the VSDSquadron
RISC-V SoC.

**Typical use cases:**
- Generating periodic interrupts/events for polling loops (heartbeat, LED blink)
- One-shot delays (timeouts, debounce windows, watchdog-style countdowns)
- Software-visible elapsed-time measurement via the `VALUE` register

**Why/when to use it:** any time firmware needs a hardware-accurate
countdown that runs independently of the CPU's instruction stream,
freeing software to poll a flag instead of manually counting cycles.

## 2. Feature Summary

| Feature | Detail |
|---|---|
| Modes | One-shot, Periodic (auto-reload) |
| Counter width | 32-bit |
| Prescaler | Optional 8-bit divider (`PRESC_DIV`), enabled via `PRESC_EN` |
| Status flag | Write-1-to-Clear (W1C), bit 0 of `STATUS` |
| Clock | Single clock domain, synchronous to system `clk` |
| Interrupt line | **Not supported** — status is polled, not an IRQ |
| Channels | Single-channel only (one instance = one timer) |

**Limitations (be honest, as commercial datasheets are):**
- No interrupt output — `timeout_o` is a level/status signal only, wired to an LED in the reference integration, not to a CPU interrupt controller.
- Single countdown channel per instance. Multiple timers require multiple instances at different base addresses.
- Reads of `VALUE` are registered (one-cycle latency), not combinational.
- No pause/resume — clearing `EN` reloads `VALUE` from `LOAD` on the next cycle rather than freezing the count.

## 3. Block Diagram

```
        CPU Bus (sel, wr_en, rd_en, addr, wdata / rdata)
                        |
                Register Decode
           (CTRL | LOAD | VALUE | STATUS)
                        |
              Counter / FSM
     (prescaler -> countdown -> reload/one-shot)
                        |
          Output / Status Flags
        (timeout_o  ->  board LED)
```

## 4. Software Programming Model

**Initialization sequence:**
1. Write `LOAD` with the desired countdown/reload value.
2. Write `CTRL` to configure mode (`MODE` bit), prescaler (`PRESC_EN`,
   `PRESC_DIV`), and set `EN = 1` to start counting.
3. Poll `STATUS` bit 0 to detect timeout (no interrupt is available).
4. On timeout, write `1` to `STATUS` bit 0 to clear the flag (W1C).
   In periodic mode, the counter has already reloaded and keeps running.

**Polling vs. status checking:** this IP is poll-only. Firmware must
spin-wait (or interleave other work) while checking `STATUS`; there is
no way to be notified asynchronously.

See `Register_Map.md` for full bit-level detail and
`Integration_Guide.md` for how to wire this IP into a SoC.
