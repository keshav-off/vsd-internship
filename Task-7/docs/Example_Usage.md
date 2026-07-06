# Timer IP — Example Usage & Validation

## 1. Board-Level Usage (VSDSquadron FPGA)

- The IP itself drives no FPGA pins directly. The only board-visible signal
  is `timeout_o`, which the reference integration wires to **LED0** through
  the top-level SoC (see `rtl/riscv_integration_snippet.v`).
- No dedicated constraint (`.pcf`/`.xdc`) entries are required for the IP
  itself, since it is purely memory-mapped logic. If you connect
  `timeout_o` to an LED, add/reuse the existing LED pin constraint already
  present in your board's constraint file — no new pin needs to be added
  for the timer.
- No external headers, buttons, or peripherals are required to exercise
  this IP; it can be fully validated with UART print statements alone.

## 2. Example Software

Three ready-to-run C programs are provided in `software/`, all built on
the same `IO_OUT`/`IO_IN` memory-mapped helper macros used elsewhere in the
VSDSquadron RISC-V firmware examples.

| File | Demonstrates |
|---|---|
| `Timer_test.c` | One-shot mode: LOAD → CTRL enable → poll VALUE → detect timeout via STATUS |
| `Timer_periodic_test.c` | Periodic (auto-reload) mode: multiple timeout/reload cycles, W1C clear inside a loop |
| `Timer_clear_test.c` | STATUS write-1-to-clear behavior in isolation, plus disabling the timer |

### Quick Start — One-Shot Mode

```c
IO_OUT(IO_TIMER_LOAD, 100000);           // 1. Load countdown value
IO_OUT(IO_TIMER_CTRL, CTRL_EN);          // 2. Enable, one-shot mode
while (!(IO_IN(IO_TIMER_STATUS) & 0x1)); // 3. Poll for timeout
printf("TIMEOUT OCCURRED!\n");           // 4. React
```

Build and flash exactly as you would any other VSDSquadron RISC-V firmware
example in this repo family (same toolchain, same `Makefile` flow used for
the UART/GPIO examples).

## 3. Validation & Expected Output

### Test: `Timer_test.c` (one-shot)
- **Expected UART output**, in order:
  - `LOAD written: 0x186a0 -> readback: 0x186a0`
  - `CTRL written -> readback: 0x1`
  - Three decreasing `VALUE = 0x...` prints
  - `TIMEOUT OCCURRED!`
  - `STATUS = 0x1`
  - `VALUE after timeout = 0x0`

### Test: `Timer_periodic_test.c` (periodic)
- **Expected UART output:** three repeated blocks of
  `Timeout #N occurred` → `VALUE after reload = 0x14` → `STATUS after clear = 0x0`,
  proving the counter reloads and re-arms without further CPU intervention
  beyond clearing the flag.

### Test: `Timer_clear_test.c` (W1C isolation)
- **Expected UART output:** `TIMEOUT OCCURRED`, then
  `STATUS after W1C clear = 0x0 (expect 0)`, then
  `STATUS after disable = 0x0 (expect 0)`.

### Common Failure Symptoms

| Symptom | Likely Cause |
|---|---|
| `STATUS` never sets (loop hangs forever) | `CTRL.EN` not written, or `LOAD` was 0 |
| `VALUE` reads back stale/wrong data | Reading `VALUE` combinationally instead of allowing the 1-cycle registered read latency |
| Timer restarts unexpectedly | `CTRL.EN` accidentally cleared elsewhere in firmware — this reloads `VALUE` from `LOAD` |
| `STATUS` won't clear | Writing anything other than `1` to bit 0 — this register is write-1-to-clear only |
| Periodic mode looks like one-shot | `CTRL.MODE` bit not set (bit 1 must be `1` for periodic) |

## 4. Known Limitations & Notes

- No interrupt support — all timeout detection is via polling `STATUS`.
- Single-channel only; instantiate multiple copies at different base
  addresses if more than one independent timer is needed.
- Assumes the SoC system clock frequency is whatever your VSDSquadron
  top-level is already running at — the timer counts system clock cycles
  (optionally divided by `PRESC_DIV`), it has no independent clock input.
- `VALUE` writes are ignored; it is a read-only, hardware-updated register.
