# Task 5: Design a Multi-Register GPIO IP with Software Control

## Objective

Extend the simple GPIO IP from Task 4 into a realistic, multi-register, software-controlled IP — similar to what exists in production SoCs. This task focuses on designing a proper register map, handling multiple registers inside one IP, and validating end-to-end control from software to hardware.

---

## IP Specification — GPIO Control IP (Direction + Data)

### Register Map

| Offset | Register Name | Address | Description |
|---|---|---|---|
| 0x00 | GPIO_DATA | 0x00400020 | GPIO output data register |
| 0x04 | GPIO_DIR | 0x00400024 | Direction register (1 = output, 0 = input) |
| 0x08 | GPIO_READ | 0x00400028 | Readback register — current pin values |

**Offset decoded via `mem_addr[3:2]`:**
- `2'b00` → GPIO_DATA
- `2'b01` → GPIO_DIR
- `2'b10` → GPIO_READ

### Functional Requirements

- **GPIO_DATA:** Writing updates output values; reading returns last written value
- **GPIO_DIR:** Each bit controls direction of corresponding GPIO pin (1 = output, 0 = input)
- **GPIO_READ:** Returns current GPIO pin values — for output pins reflects driven value, for input pins reflects actual pin state

---

## Step 1: Study and Plan

Reviewed Task 4 `gpio_ip.v` and identified what needed to be added:
- Two internal registers instead of one (`gpio_data_reg`, `gpio_dir_reg`)
- Address offset decoding via `mem_addr[3:2]` passed as `gpio_offset[1:0]`
- `gpio_read` logic combining direction and data
- New output ports: `gpio_dir`, `gpio_data`, `gpio_read`

**Screenshot: Task 4 gpio_ip.v (starting point)**

<img width="890" height="494" alt="Screenshot 2026-06-25 184925" src="https://github.com/user-attachments/assets/7ebda982-90a8-4ece-9862-d1b50f5f2ddf" />


---

## Step 2: Multi-Register GPIO IP RTL — `gpio_ip.v`

Extended `gpio_ip.v` with full multi-register support:

```verilog
/*
================================
GPIO control ip- multi register 
register map
    base + 0x00 = GPIO_DATA (0x00400020)
    base + 0x04 = GPIO_DIR  (0x00400024)
    base + 0x08 = GPIO_READ (0x00400028)
    
offset decoded via mem_addr[3:2]
2'b00 = GPIO_DATA   
2'b01 = GPIO_DIR
2'b10 = GPIO_READ
================================
*/

module gpio_ip(
       input clk,
       input resetn,
       
       input gpio_sel, 
       input gpio_we, 
       input [31:0] gpio_wdata, 
       
       input [31:0] gpio_in,      // actual pin values from outside 
       input [1:0] gpio_offset,   // new: from mem_addr[3:2]
       
       output reg [31:0] gpio_rdata, 
       
       output wire [31:0] gpio_read,
       output  [31:0] gpio_data,
       output  [31:0] gpio_dir    // new: direction output
       );
       
       // internal registers
       reg [31:0] gpio_data_reg;
       reg [31:0] gpio_dir_reg; 
       
       // Write Logic
       always@(posedge clk) begin
          if(~resetn)
          begin
             gpio_data_reg <= 32'd0;
             gpio_dir_reg <= 32'd0;
          end 
          else if(gpio_sel && gpio_we) begin
             case (gpio_offset) 
               2'b00: gpio_data_reg <= gpio_wdata; // data
               2'b01: gpio_dir_reg  <= gpio_wdata; // dir
             endcase
          end
       end
       
       //=================================================
       // GPIO_READ value 
       // for output pin (dir = 1): reflect data register value 
       // for input pin  (dir = 0): reflects actual gpio_in value 
       //=================================================
       
       assign gpio_read = (gpio_dir_reg & gpio_data_reg) | (~gpio_dir_reg & gpio_in);
       
       // Readback Logic  
       always@(*) begin
          case(gpio_offset)
          2'b00: gpio_rdata = gpio_data_reg;
          2'b01: gpio_rdata = gpio_dir_reg;
          2'b10: gpio_rdata = gpio_read;
          default: gpio_rdata = 32'b0;
          endcase
       end

       /*
       =============================================
       output assignment 
       gpio_data: the value to drive on output pins
       gpio_dir:  which pins are output 
       =============================================
       */
       
       assign gpio_data = gpio_data_reg;
       assign gpio_dir  = gpio_dir_reg;
endmodule
```

**Screenshot: gpio_ip.v full multi-register source**

<img width="899" height="819" alt="Screenshot 2026-06-26 112046" src="https://github.com/user-attachments/assets/df8f6126-0ec5-49a2-bad0-0e0e12683fcf" />


---

## Step 3: SoC Integration

### 3a. GPIO Wire Declarations in `riscv.v`

Added GPIO signal wires including new `gpio_offset`, `gpio_dir`, `gpio_data`, and `gpio_read`:

```verilog
//-- GPIO Signal --
wire gpio_sel    = isIO & mem_wordaddr[IO_GPIO_bit];
wire gpio_we     = mem_wstrb;
wire [1:0] gpio_offset = mem_addr[3:2];
wire [31:0] gpio_rdata;
wire [31:0] gpio_dir;
wire [31:0] gpio_data;
wire [31:0] gpio_read;
```

**Screenshot: GPIO wire declarations in riscv.v**

<img width="597" height="123" alt="Screenshot 2026-06-26 112124" src="https://github.com/user-attachments/assets/2b2e6924-0ec5-41db-bcb2-e32acc56cf1e" />


### 3b. GPIO IP Instantiation in `riscv.v`

```verilog
gpio_ip GPIO(
    .clk(clk),
    .resetn(resetn),
    .gpio_sel(gpio_sel),
    .gpio_we(gpio_we),
    .gpio_offset(gpio_offset),    // <- new
    .gpio_wdata(mem_wdata),
    .gpio_rdata(gpio_rdata),
    .gpio_data(gpio_data),        // <- new (replaced gpio_out)
    .gpio_dir(gpio_dir),          // <- new
    .gpio_read(gpio_read),        // <- new
    .gpio_in(32'b0)               // tied to 0 (no external input in simulation)
);
```

**Screenshot: GPIO IP instantiation in riscv.v**

<img width="655" height="168" alt="Screenshot 2026-06-26 112141" src="https://github.com/user-attachments/assets/f9999890-5eaa-41ed-9be5-bc3db970c4d0" />


---

## Step 4: Firmware — `gpio_test.c`

Used `io.h` macros (`IO_OUT`, `IO_IN`) with GPIO offsets matching the register map:

```c
#include "io.h"

#define IO_GPIO_DATA  32   // IO_BASE + 0x20
#define IO_GPIO_DIR   36   // IO_BASE + 0x24
#define IO_GPIO_READ  40   // IO_BASE + 0x28

int main() {
    printf(" ===================================\n");
    printf("TEST 1:\n");
    IO_OUT(IO_GPIO_DIR, 0xFFFFFFFF);
    printf("GPIO_DIR write 0xFFFFFFFF -> readback: 0x%x\n", IO_IN(IO_GPIO_DIR));

    IO_OUT(IO_GPIO_DATA, 0xA0A0A0A0);
    printf("GPIO_DATA write 0xA0A0A0A0 -> readback: 0x%x\n", IO_IN(IO_GPIO_DATA));

    printf("GPIO_READ -> value: 0x%x\n", IO_IN(IO_GPIO_READ));

    printf("\nTEST 2:\n");
    IO_OUT(IO_GPIO_DIR, 0x000000FF);
    printf("GPIO_DIR partial 0xFF -> readback: 0x%x\n", IO_IN(IO_GPIO_DIR));

    IO_OUT(IO_GPIO_DATA, 0xA0A0A0A0);
    printf("GPIO_READ partial -> value: 0x%x\n", IO_IN(IO_GPIO_READ));
    printf("===================================\n");

    return 0;
}
```

**Screenshot: gpio_test.c source**

<img width="804" height="542" alt="Screenshot 2026-06-26 114822" src="https://github.com/user-attachments/assets/c8f6e150-5dae-4aac-879e-8d1729305bf9" />


### Compiled with:

```bash
cd ~/riscv_fpga/vsdfpga_labs/basicRISCV/Firmware
make gpio_test.bram.hex
```

**Screenshot: Successful firmware compilation**

<img width="999" height="290" alt="Screenshot 2026-06-26 114857" src="https://github.com/user-attachments/assets/ad21da0e-93e0-47ae-a316-ccd2f73da61d" />


---

## Step 5: Simulation & Validation

### Simulation Command

```bash
cd ~/riscv_fpga/vsdfpga_labs/basicRISCV/RTL
iverilog -g2012 -DBENCH -o gpio_sim riscv.v gpio_ip.v bench.v
vvp gpio_sim
```

### Simulation Output

```
VCD info: dumpfile gpio_sim.vcd opened for output.
=== GPIO Simulation Start ===
Reset released at time 1000
===================================TEST 1:
Time=270445800 | LEDS = 11111 (0x1f)
GPIO_DIR write 0xFFFFFFFF -> readback: 0xFFFFFFFF
GPIO_DATA write 0xA0A0A0A0 -> readback: 0xA0A0A0A0
GPIO_READ -> value: 0xA0A0A0A0

TEST 2:
GPIO_DIR partial 0xFF -> readback: 0x000000FF
GPIO_READ partial -> value: 0x000000A0
===================================
$finish called at 1719744200 (100ps)
```

**Screenshot: Simulation terminal output**

<img width="883" height="184" alt="Screenshot 2026-06-26 115139" src="https://github.com/user-attachments/assets/c36cbd34-f6cc-4a2a-ad00-2711da5a6cfe" />


---

## Step 6: Waveform Analysis (GTKWave)

```bash
gtkwave gpio_sim.vcd
```

### Waveform 1 — Register Write Transactions

Shows:
- `gpio_dir[31:0]` transitions to `FFFFFFFF` when DIR written ✅
- `gpio_data[31:0]` transitions to `A0A0A0A0` when DATA written ✅
- `gpio_read[31:0]` reflects `A0A0A0A0` (output pins, dir=1) ✅
- `gpio_offset[1:0]` toggles correctly: `00`=DATA, `01`=DIR, `10`=READ ✅
- `gpio_sel` and `gpio_we` pulse correctly on each transaction ✅

**Screenshot: GTKWave — full register write transactions**

<img width="1330" height="264" alt="Screenshot 2026-06-26 115833" src="https://github.com/user-attachments/assets/f969f3d4-82de-4f33-9616-d702dfe2825c" />


### Waveform 2 — Mixed Direction (Partial Output)

Shows:
- `gpio_dir[31:0]` = `000000FF` (only lower 8 bits as output) ✅
- `gpio_read[31:0]` = `000000A0` (only lower 8 bits reflect data, upper bits = 0 since input) ✅

**Screenshot: GTKWave — mixed direction partial output**

<img width="1594" height="297" alt="Screenshot 2026-06-26 120011" src="https://github.com/user-attachments/assets/798dd779-6d1e-4cbe-812f-4603db42151e" />


---

## Understanding Check

**Q: What address map was used?**

| Register | Address | Offset |
|---|---|---|
| GPIO_DATA | 0x00400020 | 0x00 |
| GPIO_DIR | 0x00400024 | 0x04 |
| GPIO_READ | 0x00400028 | 0x08 |

Base address `0x400000` (IO_BASE) + offset `0x20` = `0x400020`. The three registers are separated by 4 bytes (one word), decoded via `mem_addr[3:2]`.

**Q: How does offset decoding work?**

`mem_addr[3:2]` gives a 2-bit offset:
- `00` → selects `gpio_data_reg` for read/write
- `01` → selects `gpio_dir_reg` for read/write
- `10` → returns `gpio_read` (computed value, read-only)

This is passed as `gpio_offset` to the IP module and used in both the write `case` and readback `case` statements.

**Q: What was validated in simulation?**

- **Test 1 (All outputs):** DIR=`0xFFFFFFFF`, DATA=`0xA0A0A0A0` → READ=`0xA0A0A0A0` ✅
- **Test 2 (Partial output):** DIR=`0x000000FF`, DATA=`0xA0A0A0A0` → READ=`0x000000A0` ✅ (only lower 8 bits driven, upper bits = 0 since configured as input with no external signal)

---

## Files Modified / Created

| File | Action |
|---|---|
| `RTL/gpio_ip.v` | Updated — extended to multi-register with offset decoding |
| `RTL/riscv.v` | Updated — added `gpio_offset`, `gpio_dir`, `gpio_data`, `gpio_read` wires and updated instantiation |
| `RTL/bench.v` | Used — simulation testbench with UART monitor |
| `Firmware/gpio_test.c` | Updated — multi-register test using `io.h` macros |
| `RTL/gpio_sim.vcd` | Generated — simulation waveform |

---

## Environment

- VSDSquadron Ubuntu VM (`vsduser@vsdsquadron`)
- Tools: `riscv64-unknown-elf-gcc`, `iverilog 11.0`, `gtkwave`
