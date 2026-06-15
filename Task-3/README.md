

## Task 3: Environment Setup & RISC-V Reference Bring-Up

**Objective:** Set up the development environment and successfully run a working RISC-V reference design, followed by running the VSDFPGA labs on the same environment.

This task focuses on:
- Toolchain readiness
- Understanding the RISC-V execution flow
- Preparing for upcoming FPGA and IP development work

---

### Step 1: GitHub Codespace Setup

Forked [vsd-riscv2](https://github.com/vsdip/vsd-riscv2) and launched a GitHub Codespace from the fork. The Codespace built successfully with the complete toolchain pre-installed.

**Toolchain versions verified inside the Codespace:**

- `riscv64-unknown-elf-gcc` — SiFive GCC 8.3.0 (2019.08.0)
- `spike` — RISC-V ISA Simulator 1.1.1-dev
- `iverilog` — Icarus Verilog version 11.0 (stable)

**Screenshot 1: RISC-V GCC & Spike toolchain verified in Codespace**

<img width="1920" height="965" alt="image" src="https://github.com/user-attachments/assets/5c1f0def-1979-478e-9a6e-5a66198eec96" />


**Screenshot 2: Icarus Verilog verified in Codespace**

<img width="1920" height="960" alt="image" src="https://github.com/user-attachments/assets/fe9cda8a-7b27-4f71-9ebb-53baa74df0e8" />


---

### Step 2: RISC-V Reference Program — Compilation & Execution

The reference C program `sum1ton.c` is located in the `samples/` directory of the `vsd-riscv2` repository.

**Program source (`sum1ton.c`):**

```c
#include <stdio.h>

int main(){
    int i, sum=0, n=9;
    for(i=1;i<=n;i++)
        sum = sum + i;
    printf("Sum from 1 to %d is %d \n", n, sum);
    return 0;
}
```

**Compilation and execution commands:**

```bash
cd samples
riscv64-unknown-elf-gcc -o sum1ton.o sum1ton.c
spike pk sum1ton.o
```

**Output (n=9):**
```
bbl loader
Sum from 1 to 9 is 45
```

**Output (n=100 — Optional Confidence Task):**
```
bbl loader
Sum from 1 to 100 is 5050
```

**Screenshot 3: RISC-V program compiled and executed successfully**

<img width="951" height="140" alt="Screenshot 2026-06-15 223312" src="https://github.com/user-attachments/assets/47d2a3d4-a8b0-4840-b8cf-6c2e2a9d6c64" />


**Screenshot 4 (Optional): Modified program with n=100 showing updated output**

<img width="1103" height="342" alt="Screenshot 2026-06-15 230424" src="https://github.com/user-attachments/assets/465812e8-25b2-46d8-9f23-6be547d3887c" />


---

### Step 3: Clone & Run VSDFPGA Labs

Cloned the `vsdfpga_labs` repository inside the Codespace and explored its structure:

```bash
git clone https://github.com/vsdip/vsdfpga_labs.git
cd vsdfpga_labs
```

**Repository structure:**

```
vsdfpga_labs/
├── basicRISCV/
│   ├── Firmware/
│   │   ├── riscv_logo.c
│   │   ├── Makefile
│   │   └── (other firmware files)
│   └── RTL/
│       ├── riscv.v
│       ├── firmware.hex
│       └── (other RTL files)
├── display_using_riscv.jpg
├── make_riscv.jpg
└── README.md
```

**Screenshot 5: vsdfpga_labs cloned and directory structure explored**

<img width="928" height="502" alt="Screenshot 2026-06-15 223635" src="https://github.com/user-attachments/assets/84478bc5-fb50-4841-a2e6-98b754e7ab69" />


---

### Step 4: VSDFPGA Firmware Build (No Hardware Required)

Built the `riscv_logo.bram.hex` firmware inside the Codespace using the RISC-V cross-compiler:

```bash
cd vsdfpga_labs/basicRISCV/Firmware
make riscv_logo.bram.hex
```

The build process:
- Cross-compiled all C and assembly sources for `rv32i` target
- Linked using `bram.ld` linker script
- Generated `.bram.hex` memory image and copied it to the RTL directory

**Key build output:**
```
RAM SIZE=6144
Code size: 780 words ( total RAM size: 1536 words )
Occupancy: 50%
SAVE HEX: riscv_logo.bram.hex
```

**Screenshot 6: Firmware build successful — riscv_logo.bram.hex generated**

<img width="916" height="685" alt="Screenshot 2026-06-15 223842" src="https://github.com/user-attachments/assets/3bbe68b1-db92-4b94-ab11-b74aea7b24d4" />


---

### Step 5: VSDFPGA Simulation Output

The `riscv_logo.c` firmware prints an ASCII banner via the RISC-V UART peripheral. Running it via `spike` produces the following output:

```
**************************************************************
*                                                            *
*    L E A R N   T O   T H I N K   L I K E   A   C H I P     *
*                                                            *
*         V S D S Q U A D R O N   F P G A   M I N I          *
*                                                            *
* B R I N G S   R I S C - V   T O   V S D   C L A S S R O O M*
*                                                            *
**************************************************************
```

**Screenshot 7: ASCII banner output from VSDFPGA firmware simulation**

<img width="1920" height="938" alt="Screenshot 2026-06-15 224307" src="https://github.com/user-attachments/assets/c034f1c9-6d9b-4bed-b2d4-279c61cad5c8" />


**Screenshot 8: riscv_logo.c source code viewed inside Codespace**

<img width="937" height="709" alt="Screenshot 2026-06-15 224743" src="https://github.com/user-attachments/assets/7b169bc6-73b9-422a-89d1-9a8c57636ea8" />


---

### Step 6: Local Machine Setup (Codespace + Local)

Both repositories were also cloned locally for upcoming FPGA board work:

```bash
mkdir ~/riscv_fpga
cd ~/riscv_fpga
git clone https://github.com/vsdip/vsd-riscv2.git
git clone https://github.com/vsdip/vsdfpga_labs.git
ls
# vsdfpga_labs  vsd-riscv2
```

Local directory structure explored:

```
~/riscv_fpga/vsdfpga_labs/basicRISCV/
├── Firmware/   ← C source, assembly, linker scripts, Makefile
└── RTL/        ← riscv.v, SOC.asc, firmware.hex, Makefile
```

**Screenshot 9: Both repositories cloned locally**

<img width="850" height="388" alt="Screenshot 2026-06-15 225437" src="https://github.com/user-attachments/assets/f9322a50-c137-4d06-a1e9-d9916c82de88" />


**Screenshot 10: Local directory structure verified (Firmware + RTL)**

<img width="922" height="479" alt="Screenshot 2026-06-15 225737" src="https://github.com/user-attachments/assets/fdeb0361-4bf8-4662-be25-b74ffdd2636a" />


---

### Understanding Check — Answers

**Q1. Where is the RISC-V program located in the vsd-riscv2 repository?**

The RISC-V reference program (`sum1ton.c`) is located in the `samples/` directory of the `vsd-riscv2` repository.

---

**Q2. How is the program compiled and loaded into memory?**

The source code is cross-compiled into a statically linked ELF binary targeting the 64-bit RISC-V architecture using the `riscv64-unknown-elf-gcc` toolchain. Execution is handled by **Spike** (the official RISC-V ISA simulator) working together with the **RISC-V Proxy Kernel (pk)**. The proxy kernel parses the ELF program headers, maps the executable segments into simulated physical memory, initializes the architectural state, and proxies system calls from the simulated hardware thread (hart) to the host OS.

```bash
riscv64-unknown-elf-gcc -o sum1ton.o sum1ton.c   # compile
spike pk sum1ton.o                                 # simulate
```

---

**Q3. How does the RISC-V core access memory and memory-mapped IO?**

The RISC-V core accesses both memory and peripherals using standard **load** and **store** instructions. Memory-Mapped IO (MMIO) allows peripherals (like UART and LEDs) to be mapped at specific address ranges. When the CPU places an address on the bus, bit 22 of the address (`mem_addr[22]`) determines whether the access targets RAM or an IO peripheral:

- `mem_addr[22] == 0` → RAM
- `mem_addr[22] == 1` → IO (peripherals)

Individual peripherals are further selected using 1-hot bits of the word-aligned address (`mem_addr[31:2]`).

---

**Q4. Where would a new FPGA IP block logically integrate in this system?**

A new FPGA IP block would integrate as a **memory-mapped peripheral** connected to the SoC interconnect in `riscv.v`. It would be assigned a unique address bit (e.g., `IO_GPIO_bit = 3`), instantiated in the SOC module, and its read data would be muxed into the `mem_rdata` bus. This allows the RISC-V CPU to communicate with the new IP using standard load/store operations — exactly as it does with LEDs and UART.

---

### Environment Used

-  GitHub Codespace (primary)
-  Local machine setup (partial — repositories cloned, tools installed natively on Linux)

> **Note:** FPGA tools (yosys, nextpnr, programmers, drivers) were **NOT** installed at this stage as per task guidelines. FPGA bring-up will be introduced in later tasks.

---

### References

- [vsd-riscv2 Repository](https://github.com/vsdip/vsd-riscv2)
- [vsdfpga_labs Repository](https://github.com/vsdip/vsdfpga_labs)
- [Dockerfile (tool reference)](https://raw.githubusercontent.com/vsdip/vsd-riscv2/refs/heads/main/.devcontainer/Dockerfile)
