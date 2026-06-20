# Task 4 — Design and Integrate a Memory-Mapped GPIO Output IP

## Overview

This task transitions from environment setup to RTL development by designing a Simple GPIO Output IP (write-only), integrating it into the existing RISC-V SoC, and validating it through simulation. The IP exposes one 32-bit register that updates an output signal on write and returns the last written value on read, accessed through the same memory-mapped bus already used by the LED and UART peripherals in the SoC.

---

## Table of Contents

- [IP Specification](#ip-specification)
- [Address Map](#address-map)
- [Step 1 — Understanding the Existing SoC](#step-1--understanding-the-existing-soc)
- [Step 2 — Writing the IP RTL](#step-2--writing-the-ip-rtl)
- [Step 3 — Integrating the IP into the SoC](#step-3--integrating-the-ip-into-the-soc)
- [Step 4 — Validation via Simulation](#step-4--validation-via-simulation)
- [Submission Summary](#submission-summary)
- [Conclusion](#conclusion)

---

## IP Specification

| Property | Detail |
|----------|--------|
| IP Name | Simple GPIO Output IP (Write-Only) |
| Register | One 32-bit register |
| Write behaviour | Updates the output signal (`gpio_out`) |
| Read behaviour | Returns the last written value (`gpio_rdata`) |
| Interface | Memory-mapped, connected to the existing CPU bus |
| Design style | Fully synchronous |

---

## Address Map

The SoC uses a 1-hot addressing scheme within its IO page. Each peripheral is assigned a bit number, and the byte offset is derived as:

```
byte_offset = (1 << bit_number) << 2
```

| Peripheral | IO Bit | Byte Offset | Full Address |
|------------|--------|-------------|--------------|
| `IO_LEDS_bit` | 0 | `0x04` | `0x00400004` |
| `IO_UART_DAT_bit` | 1 | `0x08` | `0x00400008` |
| `IO_UART_CNTL_bit` | 2 | `0x10` | `0x00400010` |
| `IO_GPIO_bit` | 3 | `0x20` | `0x00400020` |

`IO_BASE = 0x00400000`. For the GPIO IP at bit 3: `(1 << 3) << 2 = 32 = 0x20`, giving the final address **`0x00400020`**, which matches the address used directly in the C test program (`GPIO_ADDR 0x00400020`).

---

## Step 1 — Understanding the Existing SoC

Before writing any RTL, the existing SoC structure was reviewed to locate how memory-mapped peripherals are decoded and how the CPU reads and writes registers.

```bash
cd ~/riscv_fpga/vsdfpga_labs/basicRISCV/RTL
ls
cat riscv.v
```

**Screenshot — RTL Directory Listing:**

<img width="958" height="274" alt="Screenshot 2026-06-20 165844" src="https://github.com/user-attachments/assets/698397d9-0646-40dd-a2f5-ff19c359f6aa" />


**Screenshot — SOC Top-Level Module and CPU Bus Signals:**

<img width="933" height="472" alt="Screenshot 2026-06-20 170606" src="https://github.com/user-attachments/assets/c7a6423d-88e4-44fd-a2a2-cf6b6c74eb05" />


The `SOC` module instantiates the `Processor` (CPU) and connects it to a common bus consisting of `mem_addr`, `mem_rdata`, `mem_rstrb`, `mem_wdata`, and `mem_wmask`. The top bit of the address (`mem_addr[22]`) is used to distinguish memory-mapped IO (`isIO`) from regular RAM (`isRAM`).

**Screenshot — Existing IO Decode Logic (LED and UART):**

<img width="931" height="770" alt="Screenshot 2026-06-20 171151" src="https://github.com/user-attachments/assets/66cae522-10e5-420f-b962-eb588d204862" />


The existing peripherals follow a clear pattern:
- `IO_LEDS_bit = 0` — writes update the `LEDS` output register directly
- `IO_UART_DAT_bit = 1` and `IO_UART_CNTL_bit = 2` — connect to a `corescore_emitter_uart` instance for serial transmission
- All IO writes are gated by `isIO & mem_wstrb`, and `mem_wordaddr` (the word-aligned address) is used for 1-hot bit selection per peripheral

**Screenshot — Firmware-Side Register Macros (`io.h`):**

<img width="795" height="152" alt="Screenshot 2026-06-20 180512" src="https://github.com/user-attachments/assets/b5894ff4-98b2-45b0-aea9-7e3ca2973d27" />


```c
#define IO_BASE         0x400000
#define IO_LEDS         4
#define IO_UART_DAT     8
#define IO_UART_CNTL    16

#define IO_IN(port)       *(volatile uint32_t*)(IO_BASE + port)
#define IO_OUT(port,val)  *(volatile uint32_t*)(IO_BASE + port)=(val)
```

This confirmed that the CPU accesses peripherals using simple pointer dereferences to fixed addresses, with the SoC's address decoder routing the access to the correct peripheral based on the offset.

---

## Step 2 — Writing the IP RTL

A new RTL module `gpio_ip.v` was created implementing register storage, write logic, and readback logic, following the same synchronous design style as the existing peripherals.

```bash
gedit gpio_ip.v
```

**Screenshot — `gpio_ip.v` Full Module:**

<img width="895" height="708" alt="Screenshot 2026-06-20 185739" src="https://github.com/user-attachments/assets/706e1f12-faf9-419e-ae54-e153f5953cf6" />


```verilog
module gpio_ip(
    input clk,
    input resetn,

    input gpio_sel,
    input gpio_we,
    input [31:0] gpio_wdata,

    output reg [31:0] gpio_rdata,
    output [31:0] gpio_out);

    reg [31:0] gpio_reg;  // internal register

    // Write Logic
    always@(posedge clk) begin
        if(~resetn)
            gpio_reg <= 32'd0;
        else begin
            if(gpio_sel && gpio_we)
                gpio_reg <= gpio_wdata;
        end
    end

    // Readback Logic
    always@(*) begin
        if(gpio_sel)
            gpio_rdata <= gpio_reg;
        else
            gpio_rdata <= 32'd0;
    end

    // Drive External Logic
    assign gpio_out = gpio_reg;
endmodule
```

**Design notes:**

| Aspect | Implementation |
|--------|-----------------|
| Register storage | `reg [31:0] gpio_reg`, reset to `32'd0` on `~resetn` |
| Write logic | Synchronous, gated by `gpio_sel && gpio_we` on `posedge clk` |
| Readback logic | Combinational mux — returns `gpio_reg` when selected, else `0` |
| Output drive | `gpio_out` is continuously assigned from `gpio_reg`, always reflecting the last written value |

A new `IO_GPIO_bit` localparam was added to the SoC's IO page alongside the existing LED and UART bit definitions.

**Screenshot — `IO_GPIO_bit` Added to SoC:**

<img width="859" height="127" alt="Screenshot 2026-06-20 173821" src="https://github.com/user-attachments/assets/5a50a89c-d972-4667-8010-c858c7825b37" />


```verilog
localparam IO_LEDS_bit       = 0;  // W five leds
localparam IO_UART_DAT_bit   = 1;  // W data to send (8 bits)
localparam IO_UART_CNTL_bit  = 2;  // R status. bit 9: busy sending
localparam IO_GPIO_bit       = 3;
```

---

## Step 3 — Integrating the IP into the SoC

### Bus Signal Connections

New wires were added to connect the GPIO IP to the existing memory-mapped bus, following the same selection pattern used for the UART peripheral.

**Screenshot — GPIO Signal Wires:**

<img width="833" height="121" alt="Screenshot 2026-06-20 220235" src="https://github.com/user-attachments/assets/13739c57-d2bd-4745-a541-9cbf45850080" />


```verilog
//--GPIO Signal --
wire gpio_sel = isIO & mem_wordaddr[IO_GPIO_bit];
wire gpio_we = mem_wstrb;
wire [31:0] gpio_rdata;
wire [31:0] gpio_out;
```

### IP Instantiation and Read Mux Update

The `gpio_ip` module was instantiated in the SoC top level, and the existing `IO_rdata` read mux was extended to include the new GPIO bit.

**Screenshot — GPIO Instantiation and Mux Update:**

<img width="835" height="246" alt="Screenshot 2026-06-20 185615" src="https://github.com/user-attachments/assets/0c359099-5184-4327-8579-9e0fcdfaead7" />

```verilog
gpio_ip GPIO(
    .clk(clk),
    .resetn(resetn),
    .gpio_sel(gpio_sel),
    .gpio_we(gpio_we),
    .gpio_wdata(mem_wdata),
    .gpio_rdata(gpio_rdata),
    .gpio_out(gpio_out)
);

wire [31:0] IO_rdata =
            mem_wordaddr[IO_UART_CNTL_bit] ? { 22'b0, !uart_ready, 9'b0} :
            mem_wordaddr[IO_GPIO_bit] ? gpio_rdata : 32'b0;
```

**Integration summary:**

| Step | Action |
|------|--------|
| Address decode | `gpio_sel` derived from `isIO & mem_wordaddr[IO_GPIO_bit]` |
| Write enable | `gpio_we` tied to the shared `mem_wstrb` bus signal |
| Write data | `mem_wdata` passed directly into the IP |
| Read mux | `IO_rdata` extended with a new ternary branch for the GPIO bit |
| Output exposure | `gpio_out` available at the SoC boundary for future LED/board connection |

At this point, the GPIO IP is a fully integrated part of the SoC, connected through the same bus signals used by the CPU to access RAM, LEDs, and UART.

---

## Step 4 — Validation via Simulation

### Test Firmware

A C test program `gpio_test.c` was written to write three distinct test patterns to the GPIO register and print each value back via UART.

```bash
gedit gpio_test.c
```

**Screenshot — `gpio_test.c`:**

<img width="902" height="611" alt="Screenshot 2026-06-20 182150" src="https://github.com/user-attachments/assets/3216ef42-10f3-47e7-97ac-06e0c8056485" />


```c
#include <stdio.h>
#include <stdint.h>

#define GPIO_ADDR 0x00400020
volatile uint32_t *gpio = (volatile uint32_t *)GPIO_ADDR;

void main() {
    *gpio = 0xF0F0F0F0;
    printf("GPIO test 1: %x\n", (unsigned int)*gpio);

    *gpio = 0xA0A0A0A0;
    printf("GPIO test 2: %x\n", (unsigned int)*gpio);

    *gpio = 0xAAAAAAAA;
    printf("GPIO test 3: %x\n", (unsigned int)*gpio);

    printf("ALL TESTS DONE\n");
}
```

The test writes and immediately reads back three patterns — `0xF0F0F0F0`, `0xA0A0A0A0`, and `0xAAAAAAAA` — chosen because their alternating bit patterns make incorrect bit connections immediately visible in simulation.

### Firmware Build

```bash
make gpio_test.bram.hex
```

**Screenshot — Firmware Build Output:**

<img width="931" height="409" alt="Screenshot 2026-06-20 182444" src="https://github.com/user-attachments/assets/7d1a8110-5abf-4042-b460-726dca1799e4" />


**Build summary:**

| Parameter | Value |
|-----------|-------|
| RAM size | 6144 words |
| Code size | 674 words (total RAM size: 1536 words) |
| Occupancy | 43% |
| Max address | 2697 |
| Output | `gpio_test.bram.hex` |

The hex file was automatically copied to `RTL/firmware.hex` for simulation.

### Testbench

The existing `bench.v` testbench was reused, with a monitor added to track the `LEDS` output and dump simulation waveforms for GPIO verification.

```bash
cat bench.v
```

**Screenshot — `bench.v` Testbench:**

<img width="923" height="819" alt="Screenshot 2026-06-20 200647" src="https://github.com/user-attachments/assets/243aa009-a2e5-48bc-9769-b3e6a20b6918" />


Key elements of the testbench:
- `SB_HFOSC` and `SB_PLL40_CORE` — simulation models for the FPGA's internal oscillator and PLL
- `SOC uut(...)` — instantiates the full SoC under test
- `$dumpfile("gpio_sim.vcd")` and `$dumpvars(0, bench)` — enables waveform capture for GTKWave
- A reset sequence: `RESET = 1` held for 100 time units, then released, followed by a run window of 200,000,000 time units before `$finish`

### Running the Simulation

```bash
iverilog -g2012 -DBENCH -o gpio_sim riscv.v gpio_ip.v bench.v
vvp gpio_sim
```

**Screenshot — Simulation Console Output:**

<img width="860" height="119" alt="Screenshot 2026-06-20 200928" src="https://github.com/user-attachments/assets/7a0d62ca-39ae-4b5a-b5c4-1da20e40a3a6" />


**Console output:**
```
VCD info: dumpfile gpio_sim.vcd opened for output.
=== GPIO Simulation Start ===
Reset released at time 100000
GPIO test 1: F0F0F0F0
GPIO test 2: A0A0A0A0
GPIO test 3: AAAAAAAA
ALL TESTS DONE
riscv.v:286: $finish called at 47617660000 (1ps)
```

All three written values were correctly read back via UART, exactly matching what the firmware wrote — confirming both the write path and the readback path of the GPIO IP function correctly through the full CPU-to-IP bus connection.

### Waveform Verification

The generated `gpio_sim.vcd` file was opened in GTKWave to visually confirm signal-level correctness.

**Screenshot — GTKWave Waveform:**

<img width="1740" height="1012" alt="Screenshot 2026-06-20 215556" src="https://github.com/user-attachments/assets/66362e37-0f2a-4aee-83d3-cbd3aca24334" />


**Observed signal behaviour:**

| Signal | Behaviour Observed |
|--------|---------------------|
| `gpio_sel` | Pulses high exactly during each write access |
| `gpio_we` | Pulses high in sync with `gpio_sel` during writes |
| `gpio_wdata[31:0]` | Shows `F0F0F0F0`, then `A0A0A0A0` arriving on the bus during writes |
| `gpio_reg[31:0]` | Updates to each new value on the clock edge following a write |
| `gpio_rdata[31:0]` | Mirrors `gpio_reg` whenever `gpio_sel` is asserted, confirming correct readback |
| `gpio_out[31:0]` | Continuously reflects `gpio_reg`, confirming the output is always driven by the latest written value |

The waveform confirms `gpio_reg`, `gpio_rdata`, and `gpio_out` all transition to `F0F0F0F0` and later `A0A0A0A0` in lockstep with the corresponding write pulses on `gpio_sel` and `gpio_we`, validating correct synchronous register update behaviour.

---

## Submission Summary

| Requirement | Status | Evidence |
|-------------|--------|----------|
| GPIO IP RTL file | Complete | `gpio_ip.v` |
| SoC integration | Complete | `IO_GPIO_bit` localparam, `gpio_sel`/`gpio_we` wires, `gpio_ip` instantiation, `IO_rdata` mux update |
| Simulation log | Complete | Console output showing all 3 GPIO tests passed |
| Waveform screenshot | Complete | GTKWave capture of `gpio_sel`, `gpio_we`, `gpio_wdata`, `gpio_reg`, `gpio_rdata`, `gpio_out` |
| Address used | `0x00400020` (IO_BASE `0x400000` + GPIO bit-3 1-hot offset `0x20`) |
| CPU access method | Standard memory-mapped load/store via `volatile uint32_t *` pointer dereference |
| What was validated | Correct register write, correct readback of last written value, correct continuous output drive |
| Hardware validation | Not performed — simulation-only submission (optional step) |

---

 
## Understanding Check
 
**Q: What address was used for the GPIO IP?**
 
Base address `0x00400020` — this maps to IO space (`mem_addr[22] = 1`) with `mem_wordaddr[IO_GPIO_bit]` (bit 3) selecting the GPIO peripheral in the 1-hot IO decoder.
 
**Q: How does the CPU access the GPIO IP?**
 
The RISC-V CPU issues a store instruction (`sw`) to address `0x00400020`. The SoC decodes `mem_addr[22]` as IO space, then `mem_wordaddr[3]` selects the GPIO IP. The `gpio_sel` wire goes high, `gpio_we` goes high on write strobe, and `gpio_wdata = mem_wdata` is latched into `gpio_reg` on the next clock edge. On a load instruction, `gpio_rdata` returns the stored value through the `IO_rdata` mux back to the CPU.
 
**Q: What was validated in simulation?**
 
Three write-readback transactions were validated:
- Write `0xF0F0F0F0` → read back `F0F0F0F0` ✅
- Write `0xA0A0A0A0` → read back `A0A0A0A0` ✅
- Write `0xAAAAAAAA` → read back `AAAAAAAA` ✅
The GTKWave waveform confirms correct register updates, proper `gpio_sel`/`gpio_we` toggling, and correct readback through `gpio_rdata`.
 
---

## Conclusion

This task successfully designed and integrated a Simple GPIO Output IP into the existing RISC-V SoC. The IP follows the exact specification: a single 32-bit register, synchronous write logic gated by `gpio_sel` and `gpio_we`, and combinational readback logic that returns the last written value. Integration reused the SoC's existing 1-hot IO addressing scheme, placing the GPIO IP at address `0x00400020` alongside the LED and UART peripherals.

Validation was performed entirely in simulation using Icarus Verilog. A dedicated C test program wrote three distinct bit patterns to the GPIO register and read each one back via UART, with all three values (`F0F0F0F0`, `A0A0A0A0`, `AAAAAAAA`) confirmed correct in the console log. GTKWave waveform inspection further confirmed signal-level correctness, showing `gpio_reg`, `gpio_rdata`, and `gpio_out` all updating correctly in response to bus write pulses. This task demonstrates the complete IP development lifecycle — from understanding an existing SoC bus, through RTL design, integration, and simulation-based verification — mirroring how memory-mapped peripherals are developed in real semiconductor design teams.
