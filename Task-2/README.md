# Task 2 — Spike Debugger Analysis and Custom C Program Simulation on RISC-V

## Overview

This task is divided into two parts. Part 1 covers the use of the Spike ISA simulator in interactive debug mode to perform step-by-step execution and register-level inspection of the `sum1ton.o` binary compiled at `-O1` and `-Ofast` optimization levels. Part 2 involves writing a custom C program implementing a 4-bit Universal Shift Register, compiling it for RISC-V, and analyzing the generated assembly under both optimization levels.

---

## Table of Contents

- [Tools Used](#tools-used)
- [Part 1 — Spike Debugger and Register-Level Inspection](#part-1--spike-debugger-and-register-level-inspection)
- [Part 2 — Universal Shift Register Simulation](#part-2--universal-shift-register-simulation)
- [Results and Verification](#results-and-verification)
- [Conclusion](#conclusion)

---

## Tools Used

| Tool | Purpose |
|------|---------|
| `gcc` | Native C compiler |
| `riscv64-unknown-elf-gcc` | RISC-V cross-compiler |
| `spike pk` | RISC-V ISA Simulator with Proxy Kernel |
| `spike -d` | Spike in interactive debug mode |
| `riscv64-unknown-elf-objdump` | Binary disassembler |
| `gedit` | Source code editor |
| `ls -ltr` | Binary size and timestamp verification |

---

## Part 1 — Spike Debugger and Register-Level Inspection

### Overview

This part demonstrates the use of `spike -d` to interactively step through the `sum1ton.o` binary at the instruction level and inspect register values at each step, for both `-O1` and `-Ofast` compiled binaries.

---

### Step 1 — Compilation and Spike Execution

```bash
cd samples
ls -ltr

# Native GCC
gcc sum1ton.c
./a.out

# RISC-V O1 compile and simulate
riscv64-unknown-elf-gcc -O1 -mabi=lp64 -march=rv64i -o sum1ton.o sum1ton.c
spike pk sum1ton.o

# RISC-V Ofast compile and simulate
riscv64-unknown-elf-gcc -Ofast -mabi=lp64 -march=rv64i -o sum1ton.o sum1ton.c
spike pk sum1ton.o
```

**Screenshot:**

<img width="1602" height="522" alt="Screenshot 2026-06-08 015805" src="https://github.com/user-attachments/assets/a33d7e8c-220f-414a-b128-4e7733f8381c" />


**Output:**
```
Sum from 1 to 100 is 5050      ← native GCC
bbl loader
Sum from 1 to 100 is 5050      ← RISC-V O1 + Spike
bbl loader
Sum from 1 to 100 is 5050      ← RISC-V Ofast + Spike
```

---

### Step 2 — O1 Disassembly

```bash
riscv64-unknown-elf-objdump -d sum1ton.o
```

**Screenshot:**

<img width="1920" height="1080" alt="Screenshot 2026-06-07 190802" src="https://github.com/user-attachments/assets/a9a84b82-8277-472d-842b-b20659ba3f84" />


**`main` at `-O1` (address `0x10184`):**

| Address | Instruction | Operation |
|---------|-------------|-----------|
| `10184` | `addi sp,sp,-16` | Allocate 16-byte stack frame |
| `10188` | `sd ra,8(sp)` | Save return address |
| `1018c` | `li a5,100` | Load loop limit into a5 |
| `10190` | `addiw a5,a5,-1` | Decrement loop counter |
| `10194` | `bnez a5,10190` | Branch back if not zero |
| `10198` | `lui a2,0x1` | Load upper printf format string |
| `1019c` | `addi a2,a2,954` | Complete format string address |
| `101a0` | `li a1,100` | Load n=100 for printf |
| `101a4` | `lui a0,0x21` | Load upper string address |
| `101a8` | `addi a0,a0,400` | Complete string address |
| `101ac` | `jal ra,10418` | Call printf |
| `101b0` | `li a0,0` | Load return value 0 |
| `101b4` | `ld ra,8(sp)` | Restore return address |
| `101b8` | `addi sp,sp,16` | Deallocate stack frame |
| `101bc` | `ret` | Return |

**Total instructions in `main` at `-O1`: 15**

---

### Step 3 — Spike Debug Session at -O1

```bash
spike -d pk sum1ton.o
(spike) until pc 0 10184
(spike) reg 0 sp
(spike) reg 0 ra
(spike) reg 0 a5
```

**Screenshot:**

<img width="847" height="571" alt="Screenshot 2026-06-07 194920" src="https://github.com/user-attachments/assets/b97c01e4-0ab3-43fd-b6e7-1b75268a3ff8" />


**Register inspection walkthrough:**

| Step | Instruction Executed | Register | Value | Notes |
|------|---------------------|----------|-------|-------|
| `until pc 0 10184` | — | — | — | Runs to `main` entry |
| `reg 0 sp` before | — | `sp` | `0x7f7e9b50` | Initial stack pointer |
| `addi sp,sp,-16` | Stack allocated | `sp` | `0x7f7e9b40` | Decremented by 16 bytes |
| `sd ra,8(sp)` | Return address saved | `ra` | `0x000000000001010c` | Caller return address |
| `li a5,100` before | — | `a5` | `0x0000000000000000` | Register uninitialised |
| `li a5,100` after | Loop counter loaded | `a5` | `0x0000000000000064` | 0x64 = 100 decimal |

The stack pointer decreased by exactly 16 bytes (`0x7f7e9b50` → `0x7f7e9b40`), consistent with `addi sp,sp,-16`. Register `a5` correctly loaded `0x64` after `li a5,100`.

---

### Step 4 — Ofast Disassembly

**Screenshot:**

<img width="842" height="601" alt="Screenshot 2026-06-07 195447" src="https://github.com/user-attachments/assets/888594f6-7aa4-4132-8b3a-61eec507a06b" />


**`main` at `-Ofast` (address `0x100b0`):**

| Address | Instruction | Operation |
|---------|-------------|-----------|
| `100b0` | `lui a2,0x1` | Load upper format string address |
| `100b4` | `lui a0,0x21` | Load upper string address |
| `100b8` | `addi sp,sp,-16` | Allocate stack frame |
| `100bc` | `addi a2,a2,954` | Complete format string |
| `100c0` | `li a1,100` | Load n=100 |
| `100c4` | `addi a0,a0,384` | Complete string address |
| `100c8` | `sd ra,8(sp)` | Save return address |
| `100cc` | `jal ra,1040c` | Call printf |
| `100d0` | `ld ra,8(sp)` | Restore return address |
| `100d4` | `li a0,0` | Return value 0 |
| `100d8` | `addi sp,sp,16` | Deallocate stack frame |
| `100dc` | `ret` | Return |

**Total instructions in `main` at `-Ofast`: 12**

At `-Ofast`, the loop is completely eliminated. The sum `1+2+...+100 = 5050` is computed at compile time and embedded as a constant — this is called **constant folding**.

---

### Step 5 — Spike Debug Session at -Ofast

```bash
riscv64-unknown-elf-gcc -Ofast -mabi=lp64 -march=rv64i -o sum1ton.o sum1ton.c
spike -d pk sum1ton.o
(spike) until pc 0 100b0
(spike) reg 0 a2
(spike) reg 0 a0
(spike) until pc 0 100b8
(spike) reg 0 sp
```

**Screenshot:**

<img width="1920" height="1080" alt="Screenshot 2026-06-07 204628" src="https://github.com/user-attachments/assets/01881726-fdac-43a1-bbd0-bb48c0bc66dd" />


**Register inspection walkthrough:**

| Step | Instruction | Register | Value |
|------|-------------|----------|-------|
| `until pc 0 100b0` | `lui a2,0x1` before | `a2` | `0x0000000000000000` |
| After `lui a2,0x1` | — | `a2` | `0x0000000000001000` |
| After `lui a0,0x21` | — | `a0` | `0x0000000000021000` |
| `until pc 0 100b8` | Before `addi sp,sp,-16` | `sp` | `0x7f7e9b50` |
| After `addi sp,sp,-16` | — | `sp` | `0x7f7e9b40` |

The calculator in the screenshot confirms: `7F7E9B50 - 10 = 7F7E9B40`, verifying the 16-byte (0x10) stack decrement.

---

### Part 1 — O1 vs Ofast Summary

| Observation | -O1 | -Ofast |
|-------------|-----|--------|
| `main` entry address | `0x10184` | `0x100b0` |
| Instructions in `main` | 15 | 12 |
| Loop present | Yes (`bnez a5,10190`) | No — eliminated |
| Sum computed | At runtime | At compile time (constant folding) |
| Stack pointer before `main` | `0x7f7e9b50` | `0x7f7e9b50` |
| Stack pointer after allocation | `0x7f7e9b40` | `0x7f7e9b40` |
| Frame size | 16 bytes | 16 bytes |
| `a5` after `li a5,100` | `0x64` (100) | Register not used |

---

## Part 2 — Universal Shift Register Simulation

### Overview

A 4-bit Universal Shift Register was designed in C and compiled for RISC-V. The register supports Hold, Left Shift, Right Shift, and Parallel Load modes via a menu-driven interface. Assembly output at `-O1` and `-Ofast` is analyzed and compared.

---

### Register Design

The register is modelled as a 4-element integer array `q[4]` representing flip-flop outputs Q0–Q3, initialised to `{0, 0, 0, 0}`.

| Mode | Select Line | Operation |
|------|-------------|-----------|
| Hold | 0 | Register retains its current state |
| Left Shift | 1 | New data enters at Q0, bits shift toward Q3 |
| Right Shift | 2 | New data enters at Q3, bits shift toward Q0 |
| Parallel Load | 3 | All four bits loaded simultaneously |
| Exit | 4 | Program terminates |

---

### Source Code — `uni_shift_reg.c`

```c
#include <stdio.h>

int main()
{
    int sel_line;
    int data;
    int q[4] = {0, 0, 0, 0};

    while(1)
    {
        printf("\n\noperations:\n0: hold values\n1: left shift\n2: right shift\n3: parallel processing\n4: exit\nselect: ");
        scanf("%d", &sel_line);

        switch(sel_line)
        {
            case 0:
                printf("\nq0 q1 q2 q3\n%d  %d  %d  %d", q[0], q[1], q[2], q[3]);
                break;

            case 1:
                printf("\nenter data to shift: ");
                scanf("%d", &data);
                q[3] = q[2];
                q[2] = q[1];
                q[1] = q[0];
                q[0] = data;
                printf("q0 q1 q2 q3\n%d  %d  %d  %d", q[0], q[1], q[2], q[3]);
                break;

            case 2:
                printf("\nenter data to shift: ");
                scanf("%d", &data);
                q[0] = q[1];
                q[1] = q[2];
                q[2] = q[3];
                q[3] = data;
                printf("q0 q1 q2 q3\n%d  %d  %d  %d", q[0], q[1], q[2], q[3]);
                break;

            case 3:
                printf("\nenter data for parallel load: ");
                scanf("%d %d %d %d", &q[0], &q[1], &q[2], &q[3]);
                printf("q0 q1 q2 q3\n%d  %d  %d  %d", q[0], q[1], q[2], q[3]);
                break;

            case 4:
                printf("Exiting program....\n");
                return 0;

            default:
                printf("option not available\n");
        }
    }

    return 0;
}
```

---

### Step 1 — Writing the Source Code

```bash
gedit uni_shift_reg.c
```

**Screenshot:**

<img width="1920" height="1080" alt="Screenshot 2026-06-08 010753" src="https://github.com/user-attachments/assets/319f526a-1b13-4287-896b-745cb65db539" />


---

### Step 2 — Native GCC Compilation and Execution

```bash
gcc uni_shift_reg.c
./a.out
```

**Screenshot:**

<img width="1920" height="1080" alt="Screenshot 2026-06-08 010853" src="https://github.com/user-attachments/assets/55dd0db9-f10b-430e-ad0b-64523e425284" />


**Session walkthrough:**

| Input | Operation | Output |
|-------|-----------|--------|
| `0` | Hold | `q0 q1 q2 q3 = 0 0 0 0` |
| `1`, data `1` | Left Shift | `q0 q1 q2 q3 = 1 0 0 0` |
| `4` | Exit | `Exiting program....` |

---

### Step 3 — RISC-V Compilation and Spike Simulation at -O1

```bash
riscv64-unknown-elf-gcc -O1 -mabi=lp64 -march=rv64i -o uni_shift_reg.o uni_shift_reg.c
ls -ltr uni_shift_reg.o
spike pk uni_shift_reg.o
```

**Screenshot — O1 Compilation and Verification:**

<img width="1579" height="96" alt="Screenshot 2026-06-08 011425" src="https://github.com/user-attachments/assets/9a9edc54-004b-435a-a6cc-9b7b5799647c" />


```
-rwxrwxrwx 1 root root 264560 Jun 7 19:41 uni_shift_reg.o
```

**Screenshot — Spike Simulation at -O1:**

<img width="1920" height="1080" alt="Screenshot 2026-06-08 011059" src="https://github.com/user-attachments/assets/e4f47904-ea6c-40b1-be66-e96d62e0d293" />

Output matches native GCC execution exactly. `bbl loader` confirms successful proxy kernel boot.

---

### Step 4 — RISC-V Compilation and Spike Simulation at -Ofast

```bash
riscv64-unknown-elf-gcc -Ofast -mabi=lp64 -march=rv64i -o uni_shift_reg.o uni_shift_reg.c
ls -ltr uni_shift_reg.o
spike pk uni_shift_reg.o
```

**Screenshot — Ofast Compilation and Verification:**

<img width="1582" height="97" alt="Screenshot 2026-06-08 011855" src="https://github.com/user-attachments/assets/f1f4de62-0279-4d6f-9b2e-49077a862dbc" />

```
-rwxrwxrwx 1 root root 264592 Jun 7 19:48 uni_shift_reg.o
```

**Screenshot — Spike Simulation at -Ofast:**

<img width="1920" height="1080" alt="Screenshot 2026-06-08 011145" src="https://github.com/user-attachments/assets/0abd8763-3ba3-4ee2-b646-3d8257443b82" />

Output is identical to the `-O1` run, confirming functional equivalence.

---

### Step 5 — Assembly Analysis at -O1

```bash
riscv64-unknown-elf-objdump -d uni_shift_reg.o
```

**Screenshot — O1 Disassembly (Part 1):**

<img width="1920" height="1080" alt="Screenshot 2026-06-08 011619" src="https://github.com/user-attachments/assets/84df0d80-657b-4063-b150-dc8bc14d090d" />

**Screenshot — O1 Disassembly (Part 2):**

<img width="1920" height="1080" alt="Screenshot 2026-06-08 011815" src="https://github.com/user-attachments/assets/a28b43ac-39f9-4de7-9df9-425e205f3d16" />

**calculation:**
```
10334 - 10184 = 1B0
1B0 / 4 = 6C (108 in dec)
```

**Key observations at `-O1`:**

| Item | Detail |
|------|--------|
| `main` address | `0x10184` |
| Stack frame | `addi sp,sp,-96` |
| Saved registers | `ra`, `s0`–`s6` (8 registers) |
| Array zeroing | 4 × `sw zero` word stores |
| `printf` calls | `jal ra,1058c` |
| `scanf` calls | `jal ra,106c0` |
| Loop jump | `j 101f8` (back to `while` top) |
| **Total instructions** | **108** |

---

### Step 6 — Assembly Analysis at -Ofast

```bash
riscv64-unknown-elf-gcc -Ofast -mabi=lp64 -march=rv64i -o uni_shift_reg.o uni_shift_reg.c
riscv64-unknown-elf-objdump -d uni_shift_reg.o
```

**Screenshot — Ofast Disassembly (Part 1):**

<img width="1920" height="1080" alt="Screenshot 2026-06-08 011913" src="https://github.com/user-attachments/assets/587d40c8-4671-4203-87e9-30ece52f25e9" />

**Screenshot — Ofast Disassembly (Part 2):**

<img width="1920" height="1080" alt="Screenshot 2026-06-08 012040" src="https://github.com/user-attachments/assets/844d587a-bcbf-43a8-beff-55565b6a1afe" />

**calculation:**
```
10278 - 100b0 = 1C8
1C8 / 4 = 72 (114 in dec)
```

**Key observations at `-Ofast`:**

| Item | Detail |
|------|--------|
| `main` address | `0x100b0` |
| Stack frame | `addi sp,sp,-128` |
| Saved registers | `ra`, `s0`–`s9` (11 registers) |
| Array zeroing | 2 × `sd zero` double-word stores |
| Switch handling | `bltu` bounds check + jump table |
| Shift instructions | `slli`, `srli` used aggressively |
| `puts` usage | Some `printf` calls replaced with `puts` |
| **Total instructions** | **114** |

---

### Part 2 — O1 vs Ofast Summary

| Feature | -O1 | -Ofast |
|---------|-----|--------|
| `main` address | `0x10184` | `0x100b0` |
| Stack frame | 96 bytes | 128 bytes |
| Saved registers | 8 | 11 |
| Array zeroing | 4 × `sw zero` | 2 × `sd zero` |
| Switch handling | Explicit per-case branches | Bounds check + jump table |
| `printf` handling | All preserved | Some replaced with `puts` |
| Total instructions | **108** | **114** |
| Instruction reduction | — | **6% more** |
| Binary size | 264560 bytes | 264592 bytes |

### Results and Verification

| Task | Method | Instructions | Output Verified |
|------|--------|-------------|-----------------|
| Part 1 | RISC-V `-O1` + Spike | 15 | Yes |
| Part 1 | RISC-V `-Ofast` + Spike | 12 | Yes |
| Part 2 | RISC-V `-O1` + Spike | 108 | Yes |
| Part 2 | RISC-V `-Ofast` + Spike | 114 | Yes |

---

## Conclusion

This task demonstrated two complementary aspects of RISC-V toolchain analysis.

Part 1 used the Spike interactive debugger to inspect register state at the instruction level, confirming stack pointer behaviour (`0x7f7e9b50` → `0x7f7e9b40`), return address preservation, and loop counter loading. The `-Ofast` binary showed complete loop elimination through constant folding, reducing `main` from 15 to 12 instructions.

Part 2 demonstrated simulation of a 4-bit Universal Shift Register compiled for RISC-V. Contrary to expectation, -Ofast produced 114 instructions compared to 108 at -O1. This increase is due to aggressive function inlining and loop unrolling applied to the switch-case structure, where the compiler prioritizes execution speed over code size rather than minimizing instruction count.

Together, both parts provide a thorough understanding of how the RISC-V GCC toolchain transforms C programs at the assembly level across different optimization settings.
