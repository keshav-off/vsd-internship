# Timer IP — Register Map

**Base Address (example SoC mapping):** `0x00400040`
(4 registers, word-addressed, 32-bit bus)

| Offset | Register | R/W | Description |
|---|---|---|---|
| 0x00 | CTRL | R/W | Enable, mode, prescaler configuration |
| 0x04 | LOAD | R/W | Reload / initial countdown value |
| 0x08 | VALUE | R | Current countdown value (registered read) |
| 0x0C | STATUS | R/W (W1C) | Timeout flag |

---

## CTRL (offset 0x00)

| Bits | Field | Reset | R/W | Description |
|---|---|---|---|---|
| 0 | EN | 0 | R/W | 1 = timer counting, 0 = disabled (VALUE held preloaded from LOAD) |
| 1 | MODE | 0 | R/W | 0 = one-shot (stops at 0), 1 = periodic (auto-reloads from LOAD) |
| 2 | PRESC_EN | 0 | R/W | 1 = enable prescaler divider on the count clock |
| 7:3 | Reserved | 0 | — | Write 0, ignore on read |
| 15:8 | PRESC_DIV | 0 | R/W | 8-bit clock divider value, used only when `PRESC_EN=1` |
| 31:16 | Reserved | 0 | — | Write 0, ignore on read |

## LOAD (offset 0x04)

| Bits | Field | Reset | R/W | Description |
|---|---|---|---|---|
| 31:0 | LOAD_VALUE | 0x0000_0000 | R/W | Value loaded into the counter on start, on reload (periodic mode), or whenever `EN=0` |

## VALUE (offset 0x08)

| Bits | Field | Reset | R/W | Description |
|---|---|---|---|---|
| 31:0 | COUNT | 0x0000_0000 | R (read-only) | Live countdown value. One-cycle read latency (registered output). Writes are ignored. |

## STATUS (offset 0x0C)

| Bits | Field | Reset | R/W | Description |
|---|---|---|---|---|
| 0 | TIMEOUT | 0 | R/W (W1C) | Set to 1 when the counter reaches 0. Cleared by writing 1 to this bit (write-1-to-clear). Writing 0 has no effect. |
| 31:1 | Reserved | 0 | — | Ignore on read |

---

## Reset Behavior

On `resetn = 0`: `CTRL = 0`, `LOAD = 0`, `VALUE = 0`, `STATUS.TIMEOUT = 0`.
The timer does not start counting until firmware explicitly sets `CTRL.EN = 1`.

## Read/Write Timing

- Writes to `CTRL`/`LOAD` take effect on the next clock edge after `sel & wr_en`.
- `VALUE` and `STATUS` reads are registered — the data returned on a read is valid
  one clock cycle after `sel & rd_en` is asserted, matching typical memory-mapped
  peripheral behavior on this bus.
- `STATUS` write-1-to-clear is evaluated in the same cycle as the countdown
  logic; a simultaneous new timeout and a W1C clear will not race, since the
  countdown logic re-asserts `TIMEOUT` independently the next time `VALUE` hits 0.
