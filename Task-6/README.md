# Task 6: Custom Timer IP

## Objective

Design a memory-mapped Timer IP supporting one-shot mode, periodic (auto-reload) mode, and status write-1-to-clear (W1C) behavior, integrate it into the existing RISC-V SoC, and validate all three modes through simulation.

> **Reference:** The timer IP architecture and register map were studied from 
> [vsdip/vsdsquadron-fpga-ip-timer](https://github.com/vsdip/vsdsquadron-fpga-ip-timer) 
> as a design reference. The RTL, firmware, and integration were independently 
> implemented and adapted for this SoC environment.
---

## Register Map

| Offset | Register | Address | Access | Description |
|---|---|---|---|---|
| 0x00 | CTRL | 0x00400040 | R/W | Enable, mode, prescaler config |
| 0x04 | LOAD | 0x00400044 | R/W | Countdown / reload value |
| 0x08 | VALUE | 0x00400048 | R | Current counter value |
| 0x0C | STATUS | 0x0040004C | R/W1C | Timeout flag (bit 0) |

**CTRL register bit fields:**
- bit 0 — `EN` (1 = run, 0 = stop)
- bit 1 — `MODE` (0 = one-shot, 1 = periodic / auto-reload)
- bit 2 — `PRESC_EN` (prescaler enable)
- bits 15:8 — `PRESC_DIV` (prescaler divider)

**Offset decoding:** `mem_wordaddr[1:0]` → `00`=CTRL, `01`=LOAD, `10`=VALUE, `11`=STATUS

---

## Step 1: Timer IP RTL — `timer_ip.v`

```verilog
`timescale 1ns / 1ps

module timer_ip (
    input  wire        clk,
    input  wire        resetn,

    // Bus interface
    input  wire        sel,        // timer selected
    input  wire        wr_en,      // write enable
    input  wire        rd_en,      // read enable
    input  wire [1:0]  addr,       // register select
    input  wire [31:0] wdata,
    output reg  [31:0] rdata,

    // Hardware output
    output wire        timeout_o
);

    // ---------------------------------------------
    // Registers
    // ---------------------------------------------
    reg [31:0] ctrl_reg;    // CTRL
    reg [31:0] load_reg;    // LOAD
    reg [31:0] value_reg;   // VALUE
    reg        timeout;     // STATUS[0]

    // Prescaler
    reg [15:0] presc_cnt;

    // CTRL fields
    wire en        = ctrl_reg[0];
    wire mode      = ctrl_reg[1];   // 0 = one-shot, 1 = periodic
    wire presc_en  = ctrl_reg[2];
    wire [7:0] presc_div = ctrl_reg[15:8];

    // ---------------------------------------------
    // WRITE LOGIC (CTRL & LOAD only)
    // ---------------------------------------------
    always @(posedge clk) begin
        if (!resetn) begin
            ctrl_reg <= 32'b0;
            load_reg <= 32'b0;
        end else if (sel && wr_en) begin
            case (addr)
                2'b00: ctrl_reg <= wdata;   // CTRL
                2'b01: load_reg <= wdata;   // LOAD
                default: ;
            endcase
        end
    end

    // ---------------------------------------------
    // TIMER CORE LOGIC (VALUE + TIMEOUT)
    // ---------------------------------------------
    always @(posedge clk) begin
        if (!resetn) begin
            value_reg <= 32'b0;
            presc_cnt <= 16'b0;
            timeout   <= 1'b0;

        end else begin
            // STATUS W1C clear
            if (sel && wr_en && addr == 2'b11 && wdata[0])
                timeout <= 1'b0;

            if (en) begin
                // Prescaler tick
                if (!presc_en || presc_cnt == presc_div) begin
                    presc_cnt <= 16'b0;

                    if (value_reg > 1) begin
                        value_reg <= value_reg - 1;

                    end else if (value_reg == 1) begin
                        timeout <= 1'b1;
                        value_reg <= mode ? load_reg : 32'b0;

                    end else begin
                        // value_reg == 0
                        value_reg <= load_reg;
                    end
                end else begin
                    presc_cnt <= presc_cnt + 1;
                end
            end else begin
                // EN = 0 -> preload
                value_reg <= load_reg;
                presc_cnt <= 16'b0;
                timeout   <= 1'b0;
            end
        end
    end

    // ---------------------------------------------
    // READ LOGIC (registered)
    // ---------------------------------------------
    always @(posedge clk) begin
        if (!resetn) begin
            rdata <= 32'b0;
        end else if (sel && rd_en) begin
            case (addr)
                2'b00: rdata <= ctrl_reg;              // CTRL
                2'b01: rdata <= load_reg;              // LOAD
                2'b10: rdata <= value_reg;             // VALUE
                2'b11: rdata <= {31'b0, timeout};      // STATUS
                default: rdata <= 32'b0;
            endcase
        end
    end

    // ---------------------------------------------
    // Hardware output
    // ---------------------------------------------
    assign timeout_o = timeout;

endmodule
```

**Screenshot: timer_ip.v — module declaration, registers, write logic**

<img width="634" height="619" alt="Screenshot 2026-07-01 005613" src="https://github.com/user-attachments/assets/3dabd3e0-c24d-47f6-8f95-d18d3ac52e0f" />


**Screenshot: timer_ip.v — core countdown logic and read logic**

<img width="592" height="780" alt="Screenshot 2026-07-01 005803" src="https://github.com/user-attachments/assets/08b29860-47e2-4dbb-9a92-cd5b7b6dfde4" />


---

## Step 2: SoC Integration — `riscv.v`

### 2a. Address decode and wire declarations

```verilog
//-- Timer Signal--
localparam IO_TIMER_bit = 4;
localparam TIMER_BASE_WADDR = 30'h00100010;

wire timer_sel = isIO && (mem_wordaddr >= TIMER_BASE_WADDR) && (mem_wordaddr <= TIMER_BASE_WADDR +3);
wire timer_wr_en = mem_wstrb;
wire timer_rd_en = !mem_wstrb;
wire [1:0] timer_addr = mem_wordaddr[1:0];
wire [31:0] timer_rdata;
wire timer_timeout;
```

**Screenshot: Timer wire declarations in riscv.v**

<img width="905" height="209" alt="Screenshot 2026-07-01 005927" src="https://github.com/user-attachments/assets/cbe381b2-5b1a-4b9d-97ef-4ec2d3c28f27" />


### 2b. Timer IP instantiation + LED connection

```verilog
//---TIMER Instantiate--
timer_ip TIMER(
    .clk(clk),
    .resetn(resetn),
    .sel(timer_sel),
    .wr_en(timer_wr_en),
    .rd_en(timer_rd_en),
    .addr(timer_addr),
    .wdata(mem_wdata),
    .rdata(timer_rdata),
    .timeout_o(timer_timeout)
);
//----------------------------------------

always @(posedge clk) begin
    if (timer_timeout)
        LEDS[0] <= 1'b1;
end
```

**Screenshot: Timer instantiation and LED logic in riscv.v**

<img width="746" height="328" alt="Screenshot 2026-07-01 005948" src="https://github.com/user-attachments/assets/b671d77c-6888-4d78-97e7-ce237852097e" />


### 2c. IO_rdata mux updated

```verilog
wire [31:0] IO_rdata =
    mem_wordaddr[IO_UART_CNTL_bit] ? { 22'b0, !uart_ready, 9'b0} :
    mem_wordaddr[IO_GPIO_bit] ? gpio_rdata :
    timer_sel ? timer_rdata : 32'b0;
```

**Screenshot: IO_rdata mux including timer_sel**

<img width="690" height="99" alt="Screenshot 2026-07-01 010003" src="https://github.com/user-attachments/assets/05aa5975-dd6f-4df3-8cda-ee5c25aa7bed" />


---

## Step 3: Three Firmware Tests

Three separate test programs validate each independent code path of the timer: one-shot countdown, periodic auto-reload, and STATUS write-1-to-clear behavior.

### 3a. `Timer_test.c` — One-Shot Mode

```c
#include "io.h"

// ====================================================
// Timer Register Map
// Base address 0x00400040
// ====================================================
#define IO_TIMER_CTRL    64  // IO_BASE + 0x40
#define IO_TIMER_LOAD    68  // IO_BASE + 0x44
#define IO_TIMER_VALUE   72  // IO_BASE + 0x48
#define IO_TIMER_STATUS  76  // IO_BASE + 0x4C

#define TIMER_LOAD_VALUE 100000   // 0x000186A0

// CTRL bit definitions
#define CTRL_EN        (1 << 0)
#define CTRL_MODE_ONE  (0 << 1)   // one-shot mode

int main() {
    printf("=== Timer One-Shot Test ===\n");

    // Load the countdown value
    IO_OUT(IO_TIMER_LOAD, TIMER_LOAD_VALUE);
    printf("LOAD written: 0x%x -> readback: 0x%x\n",
            TIMER_LOAD_VALUE, IO_IN(IO_TIMER_LOAD));

    // Enable timer in one-shot mode
    IO_OUT(IO_TIMER_CTRL, CTRL_EN | CTRL_MODE_ONE);
    printf("CTRL written -> readback: 0x%x\n", IO_IN(IO_TIMER_CTRL));

    // Poll VALUE register a few times to observe countdown
    printf("VALUE = 0x%x\n", IO_IN(IO_TIMER_VALUE));
    printf("VALUE = 0x%x\n", IO_IN(IO_TIMER_VALUE));
    printf("VALUE = 0x%x\n", IO_IN(IO_TIMER_VALUE));

    // Poll STATUS until timeout occurs
    while (!(IO_IN(IO_TIMER_STATUS) & 0x1)) {
        // wait for timeout flag
    }

    printf("TIMEOUT OCCURRED!\n");
    printf("STATUS = 0x%x\n", IO_IN(IO_TIMER_STATUS));
    printf("VALUE after timeout = 0x%x\n", IO_IN(IO_TIMER_VALUE));

    printf("=== Test Done ===\n");

    return 0;
}
```

**Screenshot: Timer_test.c source**

<img width="662" height="578" alt="Screenshot 2026-07-01 010223" src="https://github.com/user-attachments/assets/4694c404-7fb8-4ce2-8836-65d5e2b658c9" />


**Compilation:**
```bash
make Timer_test.bram.hex
```

**Screenshot: Successful one-shot firmware build**

<img width="1006" height="288" alt="Screenshot 2026-07-01 010252" src="https://github.com/user-attachments/assets/9975386a-01d0-42c3-87f9-2c3e7c76ffdf" />


**Simulation:**
```bash
iverilog -g2012 -DBENCH -o timer_sim riscv.v timer_ip.v gpio_ip.v bench.v
vvp -n timer_sim
```

**Output:**
```
=== TIMER Simulation Start ===
Reset released at time 100000
=== Timer One-Shot Test ===
Time=18144300000 | LEDS = 00000 (0x0)
LOAD written: 0x000186A0 -> readback: 0x000186A0
CTRL written: Time=49364420000 | LEDS = 00001 (0x1)
tten -> readback: 0x00000001
VALUE = 0x000150F1
VALUE = 0x0000056F
VALUE = 0x0000E38E
TIMEOUT OCCURRED!
STATUS = 0x00000001
VALUE after timeout = 0x0001053E
=== Test Done ===
riscv.v:286: $finish called at 146999420000 (1ps)
```

**Screenshot: One-shot simulation output**

<img width="823" height="217" alt="Screenshot 2026-07-01 010353" src="https://github.com/user-attachments/assets/7764dad1-5112-4c70-a8a9-d5dbb1d65ff0" />


**Waveform — Timer load:**

<img width="1057" height="369" alt="Screenshot 2026-07-01 011459" src="https://github.com/user-attachments/assets/ed1af7bc-9ab8-400f-986e-f0934caee399" />


**Waveform — countdown reaching 1 and timeout latching:**

<img width="1064" height="337" alt="Screenshot 2026-07-01 011316" src="https://github.com/user-attachments/assets/f28bb626-c498-405a-bd0e-85f36e4644af" />



---

### 3b. `Timer_periodic_test.c` — Periodic / Auto-Reload Mode

```c
#include "io.h"

// ====================================================
// Timer Register Map
// Base address 0x00400040
// ====================================================
#define IO_TIMER_CTRL    64
#define IO_TIMER_LOAD    68
#define IO_TIMER_VALUE   72
#define IO_TIMER_STATUS  76

#define TIMER_LOAD_VALUE 20   // small value to see multiple reloads in sim

// CTRL bit definitions
#define CTRL_EN          (1 << 0)
#define CTRL_MODE_PERIOD (1 << 1)   // periodic / auto-reload mode

int main() {
    printf("=== Timer Periodic (Auto-Reload) Test ===\n");

    // Load the reload value
    IO_OUT(IO_TIMER_LOAD, TIMER_LOAD_VALUE);
    printf("LOAD written: 0x%x -> readback: 0x%x\n",
            TIMER_LOAD_VALUE, IO_IN(IO_TIMER_LOAD));

    // Enable timer in periodic mode
    IO_OUT(IO_TIMER_CTRL, CTRL_EN | CTRL_MODE_PERIOD);
    printf("CTRL written -> readback: 0x%x\n", IO_IN(IO_TIMER_CTRL));

    // Observe multiple timeout + reload cycles
    int i;
    for (i = 0; i < 3; i++) {
        // wait for timeout
        while (!(IO_IN(IO_TIMER_STATUS) & 0x1)) {
            // poll
        }
        printf("Timeout #%d occurred\n", i + 1);
        printf("VALUE after reload = 0x%x\n", IO_IN(IO_TIMER_VALUE));

        // Clear timeout flag (write-1-to-clear)
        IO_OUT(IO_TIMER_STATUS, 0x1);
        printf("STATUS after clear = 0x%x\n", IO_IN(IO_TIMER_STATUS));
    }

    printf("=== Test Done ===\n");

    return 0;
}
```

**Screenshot: Timer_periodic_test.c source**

<img width="804" height="588" alt="Screenshot 2026-07-01 014153" src="https://github.com/user-attachments/assets/70942932-d721-494e-a4ff-2bbc30920c17" />


**Compilation:**
```bash
make Timer_periodic_test.bram.hex
```

**Screenshot: Periodic test build**

<img width="1081" height="289" alt="Screenshot 2026-07-01 011846" src="https://github.com/user-attachments/assets/4066d72d-c50c-4588-a422-aebc43231a0a" />


**Simulation Output:**
```
=== TIMER Simulation Start ===
Reset released at time 100000
=== Timer Periodic (Auto-Reload) Test ===
Time=25902380000 | LEDS = 10100 (0x14)
LOAD written: 0x00000014 -> readback: 0x00000014
Time=53123140000 | LEDS = 10101 (0x15)
CTRL written -> readback: 0x00000003
Timeout #1 occurred
VALUE after reload = 0x00000002
Time=102572260000 | LEDS = 00001 (0x1)
STATUS after clear = 0x00000001
Timeout #2 occurred
VALUE after reload = 0x00000005
STATUS after clear = 0x00000000
Timeout #3 occurred
VALUE after reload = 0x00000008
STATUS a=== TIMER Simulation End ===
bench.v:64: $finish called at 200000100000 (1ps)
```

**Screenshot: Periodic mode simulation output — 3 timeouts and reloads observed**

<img width="881" height="258" alt="Screenshot 2026-07-01 012242" src="https://github.com/user-attachments/assets/2cfdc4db-e687-4f19-aaf9-022a351bd14a" />


**Waveform — Timer Load:**

<img width="770" height="239" alt="Screenshot 2026-07-01 012549" src="https://github.com/user-attachments/assets/b4dae078-fc46-4329-ac00-ffedc91ed682" />


**Waveform — continuous countdown across multiple reload cycles:**

<img width="1528" height="192" alt="Screenshot 2026-07-01 013210" src="https://github.com/user-attachments/assets/a8078caa-f073-4193-940d-8bb6f7a0ad2d" />

**Waveform — value_reg reloading from 0 back to load value (0x14) after each timeout:**

<img width="1530" height="187" alt="Screenshot 2026-07-01 013326" src="https://github.com/user-attachments/assets/66f3a4a4-0937-40ff-bac3-18db01cf1b5a" />

**Waveform — register address access pattern during polling:**

<img width="895" height="272" alt="Screenshot 2026-07-01 013930" src="https://github.com/user-attachments/assets/ee6ff867-eab2-4f03-ba5c-c368579533f7" />

**Waveform — Timeout Toggle:**

<img width="889" height="249" alt="Screenshot 2026-07-01 014044" src="https://github.com/user-attachments/assets/1cc3606f-18f5-49c2-b737-0e8a9fa2aee0" />


---

### 3c. `Timer_clear_test.c` — STATUS Write-1-to-Clear (W1C)

```c
#include "io.h"

// ====================================================
// Timer Register Map
// Base address 0x00400040
// ====================================================
#define IO_TIMER_CTRL    64
#define IO_TIMER_LOAD    68
#define IO_TIMER_VALUE   72
#define IO_TIMER_STATUS  76

#define TIMER_LOAD_VALUE 10   // 0x0A

// CTRL bit definitions
#define CTRL_EN        (1 << 0)
#define CTRL_MODE_ONE  (0 << 1)   // one-shot mode

int main() {
    printf("=== Timer STATUS Clear (W1C) Test ===\n");

    // Load countdown value
    IO_OUT(IO_TIMER_LOAD, TIMER_LOAD_VALUE);
    printf("LOAD written: 0x%x -> readback: 0x%x\n",
            TIMER_LOAD_VALUE, IO_IN(IO_TIMER_LOAD));

    // Enable timer, one-shot
    IO_OUT(IO_TIMER_CTRL, CTRL_EN | CTRL_MODE_ONE);
    printf("CTRL written -> readback: 0x%x\n", IO_IN(IO_TIMER_CTRL));

    // Wait for timeout
    while (!(IO_IN(IO_TIMER_STATUS) & 0x1)) {
        // poll
    }
    printf("TIMEOUT OCCURRED -> STATUS = 0x%x\n", IO_IN(IO_TIMER_STATUS));

    // Clear the timeout flag (write 1 to clear)
    IO_OUT(IO_TIMER_STATUS, 0x1);
    printf("STATUS after W1C clear = 0x%x (expect 0)\n", IO_IN(IO_TIMER_STATUS));

    // Verify flag stays cleared (timer disabled, no new timeout)
    IO_OUT(IO_TIMER_CTRL, 0x0);   // disable timer
    printf("STATUS after disable = 0x%x (expect 0)\n", IO_IN(IO_TIMER_STATUS));

    printf("=== Test Done ===\n");

    return 0;
}
```

**Screenshot: Timer_clear_test.c source**

<img width="686" height="572" alt="Screenshot 2026-07-01 014219" src="https://github.com/user-attachments/assets/e3165401-8cae-4e32-8f99-0b9f02c4eed8" />


**Compilation:**
```bash
make Timer_clear_test.bram.hex
```

**Screenshot: Clear test build**

<img width="1064" height="292" alt="Screenshot 2026-07-01 014247" src="https://github.com/user-attachments/assets/7c54f420-5cb3-4b39-b2bf-51b82c12b428" />

**Simulation Output:**
```
=== TIMER Simulation Start ===
Reset released at time 100000
=== Timer STATUS Clear (W1C) Test ===
Time=23685740000 | LEDS = 01010 (0xa)
LOAD written: 0x0000000A -> readback: 0x0000000A
Time=50906100000 | LEDS = 01011 (0xb)
CTRL written -> readback: 0x00000001
TIMEOUT OCCURRED -> STATUS = 0x00000001
Time=93671140000 | LEDS = 00001 (0x1)
STATUS after W1C clear = 0x00000001 (expect 0)
STATUS after disable = 0x00000000 (expect 0)
=== Test Done ===
riscv.v:286: $finish called at 154733420000 (1ps)
```

**Screenshot: W1C clear test simulation output**

<img width="894" height="193" alt="Screenshot 2026-07-01 014802" src="https://github.com/user-attachments/assets/af3a4835-b4ea-44e1-b9ad-756dd7335c06" />

**Waveform — LOAD register written (0x0A) before timer enabled:**

<img width="843" height="175" alt="Screenshot 2026-07-01 015046" src="https://github.com/user-attachments/assets/b1da93a9-aa96-4d54-92aa-ed2b9f2600f4" />

**Waveform — countdown progressing and timeout flag asserting at zero:**

<img width="1554" height="216" alt="Screenshot 2026-07-01 015257" src="https://github.com/user-attachments/assets/6a9394ae-f5a2-407f-85d5-da6ee94686f8" />

**Waveform — LEDS[4:0] reflecting timer-driven LED bit 0 toggling with countdown:**

<img width="772" height="136" alt="Screenshot 2026-07-01 020548" src="https://github.com/user-attachments/assets/40db5343-12c1-40af-8885-aa9463b7c64c" />

---

## Understanding Check

**Q: What address map was used?**

| Register | Address | Word Offset |
|---|---|---|
| CTRL | 0x00400040 | mem_wordaddr = 0x100010 |
| LOAD | 0x00400044 | mem_wordaddr = 0x100011 |
| VALUE | 0x00400048 | mem_wordaddr = 0x100012 |
| STATUS | 0x0040004C | mem_wordaddr = 0x100013 |

Decoded as a 4-word range using `TIMER_BASE_WADDR` with `>=` and `<= +3` comparison rather than 1-hot bit decoding (since 4 registers need 2 select bits, not 1 bit).

**Q: How does the prescaler work?**

When `presc_en = 1`, an internal `presc_cnt` increments every clock cycle. The main `value_reg` only decrements once `presc_cnt` reaches `presc_div`, after which `presc_cnt` resets to 0. This effectively divides the timer's tick rate by `(presc_div + 1)`, allowing longer delays without increasing register width. When `presc_en = 0`, `value_reg` decrements every clock cycle directly.

**Q: Why three separate firmware files?**

Each firmware exercises an independent RTL code path:
- `Timer_test.c` validates the one-shot branch (`mode=0` → `value_reg` goes to 0 and stays there after timeout)
- `Timer_periodic_test.c` validates the auto-reload branch (`mode=1` → `value_reg` reloads from `load_reg` repeatedly)
- `Timer_clear_test.c` validates the STATUS W1C write path (`addr==2'b11 && wdata[0]` clearing `timeout`)

Testing these independently makes debugging easier and ensures full coverage of the IP's functional requirements.

**Q: What was validated in simulation?**

| Test | Result |
|---|---|
| One-shot countdown | LOAD=0x186A0 → counted down → TIMEOUT, STATUS=1, VALUE held |
| Periodic auto-reload | LOAD=0x14 → 3 consecutive timeouts observed, VALUE correctly reloaded each time |
| STATUS W1C | TIMEOUT set STATUS=1 → write 1 cleared STATUS=0 → confirmed via readback |
| LED hardware output | `LEDS[0]` toggled in sync with `timer_timeout` signal, visible in all three simulation logs |

---

## Files Modified / Created

| File | Action |
|---|---|
| `RTL/timer_ip.v` | Created — multi-register Timer IP RTL |
| `RTL/riscv.v` | Modified — added timer address decode, instantiation, IO_rdata mux, LED connection |
| `Firmware/Timer_test.c` | Created — one-shot mode test |
| `Firmware/Timer_periodic_test.c` | Created — periodic/auto-reload mode test |
| `Firmware/Timer_clear_test.c` | Created — STATUS W1C test |

---

## Environment

- VSDSquadron Ubuntu VM (`vsduser@vsdsquadron`)
- Tools: `riscv64-unknown-elf-gcc`, `iverilog 11.0`, `gtkwave`
