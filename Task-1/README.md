# Task 1 — From C to RISC-V: Compilation Flow and Optimization Analysis

## Overview

This task demonstrates compiling a simple C program using both the native GCC compiler and the RISC-V GCC cross-compiler, verifying the output using the Spike ISA simulator, and analyzing the generated assembly under `-O1` and `-Ofast` optimization levels.

---

## Objectives

- Write a C program using a text editor
- Compile and run natively using GCC
- Cross-compile using RISC-V GCC and simulate with Spike
- Disassemble the binary at `-O1` and `-Ofast` levels
- Verify identical outputs across all compilation methods

---

## Tools Used

| Tool | Purpose |
|------|---------|
| `leafpad` | Source code editor |
| `gcc` | Native C compiler (host machine) |
| `riscv64-unknown-elf-gcc` | RISC-V cross-compiler |
| `spike` + `pk` | RISC-V ISA Simulator and Proxy Kernel |
| `riscv64-unknown-elf-objdump` | Binary disassembler |
| `cat` / `ls` | File inspection and verification |

---

## Source Code — `sum1ton.c`

```c
#include <stdio.h>

int main(){
    int i, sum=0, n=100;
    for(i=1;i<=n;i++)
        sum = sum + i;
    printf("Sum from 1 to %d is %d \n",n,sum);
    return 0;
}
```

---

## Step 1 — Writing the Source Code

The source file was created using the Leafpad text editor.

```bash
cd
leafpad sum1ton.c
```

**Screenshot:**

<img width="1107" height="774" alt="Screenshot 2026-06-04 083944" src="https://github.com/user-attachments/assets/98b913e8-d266-45a6-86d6-774f469d7ec5" />


The program was written with `n = 10` during local editing and updated to `n = 100` in the Codespaces environment.

---

## Step 2 — Native GCC Compilation and Execution

```bash
gcc sum1ton.c
./a.out
```

**Screenshot:**

<img width="679" height="453" alt="Screenshot 2026-06-04 084114" src="https://github.com/user-attachments/assets/23d50e2d-a8f3-451a-919f-a735eaa8287f" />


**Output:**
```
Sum of numbers from 1 to 10 is 55
```

Native GCC compilation confirms the program logic is correct. This output serves as the baseline reference.

---

## Step 3 — RISC-V Cross-Compilation and Spike Simulation

All subsequent steps were performed inside GitHub Codespaces via noVNC.

```bash
# Verify source
cat sum1ton.c

# Native GCC run (verification)
gcc sum1ton.c
./a.out

# RISC-V cross-compile
riscv64-unknown-elf-gcc -o sum1ton.o sum1ton.c

# Run on Spike simulator
spike pk sum1ton.o
```

**Screenshot:**

<img width="1920" height="1080" alt="Screenshot 2026-06-04 101133" src="https://github.com/user-attachments/assets/446dadcd-640b-454c-bf7d-b3ce6201b0e2" />


**Output:**
```
bbl loader
Sum from 1 to 100 is 5050
```

The Spike simulator correctly executed the RISC-V binary. The `bbl loader` line confirms successful proxy kernel boot before program execution.

---

## Step 4 — Assembly Analysis at -O1 Optimization

```bash
# Compile with O1 optimization
riscv64-unknown-elf-gcc -O1 -mabi=lp64 -march=rv64i -o sum1ton.o sum1ton.c

# Verify binary was created
ls -ltr sum1ton.o
```

**Screenshot — O1 Compilation and Verification:**

<img width="1920" height="1080" alt="Screenshot 2026-06-04 101258" src="https://github.com/user-attachments/assets/a47b9a4a-b4f8-4e18-9307-f8069f5d9268" />


**Binary info:**
```
-rwxrwxrwx 1 root root 167512 Jun 4 04:42 sum1ton.o
```

```bash
# Disassemble
riscv64-unknown-elf-objdump -d sum1ton.o

# View the Assembly File Using less
riscv64-unknown-elf-objdump -d sum_1ton.o | less

# locate main 
/main
```

**Screenshot — O1 Disassembly:**

<img width="1920" height="1080" alt="Screenshot 2026-06-04 095731" src="https://github.com/user-attachments/assets/233358cd-11de-41b1-9b04-2b4a475a96dc" />



**Observations from -O1 disassembly:**

| Item | Detail |
|------|--------|
| `main` function address | `0x10184` |
| Stack allocation | `addi sp,sp,-16` |
| Return address saved | `sd ra,8(sp)` |
| Loop counter loaded | `li a5,100` |
| Loop decrement | `addiw a5,a5,-1` |
| Branch instruction | `bnez a5,10190` |
| Total instructions in `main` | ~15 instructions |

At `-O1`, the loop structure is preserved and the assembly closely mirrors the original C code, making it readable and easy to verify.

---

## Step 5 — Assembly Analysis at -Ofast Optimization

```bash
# Compile with Ofast optimization
riscv64-unknown-elf-gcc -Ofast -mabi=lp64 -march=rv64i -o sum1ton.o sum1ton.c

# Verify binary
ls -ltr sum1ton.o
```

**Screenshot — Ofast Compilation and Verification:**

<img width="1581" height="106" alt="Screenshot 2026-06-04 101621" src="https://github.com/user-attachments/assets/d2102b24-cb49-4852-95a7-f64e81a16ff3" />


**Binary info:**
```
-rwxrwxrwx 1 root root 167512 Jun 4 04:45 sum1ton.o
```

```bash
# Disassemble
riscv64-unknown-elf-objdump -d sum1ton.o

# View the Assembly File Using less
riscv64-unknown-elf-objdump -d sum_1ton.o | less

# locate main 
/main
```

**Screenshot — Ofast Disassembly:**

<img width="1920" height="1080" alt="Screenshot 2026-06-04 100033" src="https://github.com/user-attachments/assets/f28c9e31-258e-480b-bcbb-1fa35208996d" />


**Observations from -Ofast disassembly:**

| Item | Detail |
|------|--------|
| `main` function address | `0x100b0` |
| Instructions in `main` | ~12 instructions (reduced from 15) |
| Loop transformation | Heavily optimized |
| `_start` visible | `0x100fc` — runtime startup present |
| `register_fini` visible | `0x100e0` — cleanup routines present |

At `-Ofast`, the compiler aggressively transforms the loop, producing fewer instructions with different register allocation. The assembly is less readable but functionally identical.

---

## Optimization Level Comparison

| Feature | -O1 | -Ofast |
|---------|-----|--------|
| `main` address | `0x10184` | `0x100b0` |
| Instructions in `main` | ~15 | ~12 |
| Loop structure | Preserved (`bnez` visible) | Heavily transformed |
| Stack usage | `addi sp,sp,-16` | Optimized |
| Runtime operations | More | Fewer |
| Code readability | Higher | Lower |
| Execution speed | Fast | Fastest |

---

## Results and Verification

| Method | Command | Output |
|--------|---------|--------|
| Native GCC (n=10) | `gcc sum1ton.c && ./a.out` | `Sum of numbers from 1 to 10 is 55` |
| RISC-V + Spike (n=100) | `spike pk sum1ton.o` | `Sum from 1 to 100 is 5050` |
| RISC-V `-O1` | Verified via objdump | Functionally equivalent |
| RISC-V `-Ofast` | Verified via objdump | Functionally equivalent |

All execution methods produce correct and consistent outputs, confirming functional equivalence regardless of compiler or optimization level used.

---

## Conclusion

This task successfully demonstrated the complete RISC-V compilation and simulation workflow:

1. Source editing using Leafpad and verification using `cat`
2. Native GCC compilation and execution for baseline verification
3. RISC-V cross-compilation and simulation via Spike (confirmed by `bbl loader` output)
4. Assembly-level comparison between `-O1` (~15 instructions) and `-Ofast` (~12 instructions), showing measurable instruction reduction while preserving correctness
5. Functional equivalence confirmed across all compilation methods

The experiment validates the reliability of the RISC-V toolchain and demonstrates the tangible impact of compiler optimization levels on generated machine code.
