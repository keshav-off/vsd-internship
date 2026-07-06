# Timer IP — Integration Guide

This guide answers: **"How do I plug this IP into my VSDSquadron SoC?"**
It assumes you are already familiar with the VSDSquadron FPGA RISC-V SoC
top-level (`riscv.v`) and its memory-mapped IO bus convention.

## 1. Required RTL Files

Copy the following file into your project's RTL sources:

```
rtl/timer_ip.v
```

`rtl/riscv_integration_snippet.v` is **not** a module to instantiate — it is
a reference showing the exact blocks to copy into your own `riscv.v` (see
below). Do not add it to your file list as-is.

## 2. Where to Instantiate the IP

Instantiate `timer_ip` at the SoC top level (`riscv.v`), alongside the other
memory-mapped peripherals (UART, GPIO), inside the same `always @(posedge
clk)` IO region.

```verilog
timer_ip TIMER (
    .clk       (clk),
    .resetn    (resetn),
    .sel       (timer_sel),
    .wr_en     (timer_wr_en),
    .rd_en     (timer_rd_en),
    .addr      (timer_addr),
    .wdata     (mem_wdata),
    .rdata     (timer_rdata),
    .timeout_o (timer_timeout)
);
```

## 3. Address Decoding Expectations

The IP occupies **4 consecutive 32-bit words** (16 bytes) starting at a
base address you choose. Example base: `0x00100010` (word address),
i.e. byte address `0x00400040`.

```verilog
localparam TIMER_BASE_WADDR = 30'h00100010;

wire timer_sel  = isIO && (mem_wordaddr >= TIMER_BASE_WADDR) &&
                          (mem_wordaddr <= TIMER_BASE_WADDR + 3);
wire timer_wr_en = mem_wstrb;
wire timer_rd_en = !mem_wstrb;
wire [1:0] timer_addr = mem_wordaddr[1:0];   // selects CTRL/LOAD/VALUE/STATUS
```

Adjust `TIMER_BASE_WADDR` to any free 4-word slot in your existing IO map —
just make sure it does not overlap the UART, GPIO, or other IP's decoded range.

## 4. Signals Exposed to Top-Level

| Signal | Direction | Width | Purpose |
|---|---|---|---|
| `sel` | in | 1 | Address-decoded chip select |
| `wr_en` | in | 1 | Bus write strobe |
| `rd_en` | in | 1 | Bus read strobe |
| `addr` | in | 2 | Register select (0–3 → CTRL/LOAD/VALUE/STATUS) |
| `wdata` | in | 32 | Write data from CPU store |
| `rdata` | out | 32 | Read data returned to CPU load |
| `timeout_o` | out | 1 | Hardware status flag — wire to an LED or leave unconnected |

## 5. Read-Data Mux

Add `timer_sel` as one more input to your existing `IO_rdata` mux so CPU
loads from the timer's address range return `timer_rdata`:

```verilog
wire [31:0] IO_rdata =
    mem_wordaddr[IO_UART_CNTL_bit] ? { 22'b0, !uart_ready, 9'b0} :
    mem_wordaddr[IO_GPIO_bit]      ? gpio_rdata :
    timer_sel                     ? timer_rdata : 32'b0;
```

## 6. Pin Connections (Optional)

`timeout_o` is a plain logic signal — connecting it to a physical pin is
optional and only needed if you want a visual/hardware indication of
timeout independent of firmware. See `docs/IP_User_Guide.md` §7 (Board-Level
Usage) for the LED wiring example.

## 7. Full Reference Snippet

The complete copy-paste block (localparams, wire declarations, instantiation,
LED hookup, and mux entry) is provided as-is in
`rtl/riscv_integration_snippet.v` for convenience.
