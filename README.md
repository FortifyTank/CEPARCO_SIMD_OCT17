# Euclidean Norm: C vs x86-64 vs SIMD (XMM / YMM)

Computes the Euclidean norm of a double-precision vector `A` of length `n` using four kernels, then compares their correctness and speed.

$$Z = \sqrt{\sum_{i=0}^{n-1} A_i^2}$$

## Files

| File | Description |
|---|---|
| `Euclid_C.c` | Main program, C kernel (reference answer), timing, and correctness checks |
| `Euclid_ASM.asm` | Scalar x86-64 kernel |
| `Euclid_XMM.asm` | SIMD kernel using XMM registers, 2 doubles per iteration |
| `Euclid_YMM.asm` | SIMD kernel using YMM registers, 4 doubles per iteration |

## How to Run

Open `SIMD Project Oct17.slnx` in Visual Studio 2026 and set the platform to **x64**. Then run with Ctrl+F5, once in Debug and once in Release.

## Test Setup

- **Machine:** _(CPU model, RAM)_
- **Data:** `A[i] = sin(i * 0.001)`
- **Vector sizes:** n = 2²⁰, 2²⁶, 2³⁰. If 2³⁰ does not fit in free RAM, the program falls back to 2²⁹, then 2²⁸.
- **Timing:** Only the kernel call is timed, using `QueryPerformanceCounter`. Each kernel is run once untimed as a warm-up, then the average of 30 runs is reported.
- **Correctness:** The C kernel is the answer key. Each assembly kernel is marked correct if its result is within a relative difference of 1e-9 of C.

## Boundary Handling

The SIMD loop only runs up to `n` rounded down to a multiple of 2 (XMM) or 4 (YMM). A scalar loop then processes the leftover elements. For example, with n = 1003 the YMM kernel processes 1000 elements 4 at a time and the last 3 one at a time.

---

# Report

## i. Program Output (Execution Time)

**Debug**
<img width="1200" height="497" alt="debug_output" src="https://github.com/user-attachments/assets/15f261a2-d639-4f49-b623-c2a87b601e92" />

**Release**

<img width="1919" height="1079" alt="Matt_SS" src="https://github.com/user-attachments/assets/e82b3eaa-6b19-4dfc-9d7b-cb5070c18abe" />

## ii. Execution Time and Comparison

Average execution time in ms over 30 runs.

**Debug**

| n | C | x86-64 | XMM | YMM |
|---|---|---|---|---|
| 2²⁰ | 3.2420 | 1.3198 | 0.8691 | 0.6120 |
| 2²⁶ | 197.3613 | 80.3425 | 46.4070 | 34.8181 |
| 2³⁰ | 2802.3404 | 1293.7996 | 853.9873 | 682.4332 |

**Release**

| n | C | x86-64 | XMM | YMM |
|---|---|---|---|---|
| 2²⁰ | | | | |
| 2²⁶ | | | | |
| 2³⁰ | | | | |

**Speedup over C (Debug)**

| n | x86-64 | XMM | YMM |
|---|---|---|---|
| 2²⁰ | 2.46x | 3.73x | 5.30x |
| 2²⁶ | 2.46x | 4.25x | 5.67x |
| 2³⁰ | 2.17x | 3.28x | 4.11x |

**Speedup over x86-64 (Debug)**

| n | Data size | XMM | YMM | YMM vs XMM |
|---|---|---|---|---|
| 2²⁰ | 8 MB | 1.52x | 2.16x | 1.42x |
| 2²⁶ | 512 MB | 1.73x | 2.31x | 1.33x |
| 2³⁰ | 8 GB | 1.52x | 1.90x | 1.25x |

**Why the kernels differ:**

- **x86-64 vs Debug C:** Debug builds are not optimized, so the C loop stores `i` and `sum` to memory and reads them back on every iteration. The assembly version keeps everything in registers, which makes it about 2.2–2.5x faster.
- **XMM vs x86-64:** XMM squares and adds 2 doubles per instruction, so the loop runs half as many times.
- **YMM vs x86-64:** YMM handles 4 doubles per instruction, so the loop runs a quarter as many times. YMM is the fastest kernel at every size.

## iii. Correctness Check (C)

<img width="520" height="327" alt="check_c" src="https://github.com/user-attachments/assets/fad8241c-62f1-4233-90ad-1dd5ed556f74" />

C is the answer key, so its Z values are what the other kernels are checked against.

## iv. Correctness Check (x86-64)

<img width="520" height="327" alt="check_asm" src="https://github.com/user-attachments/assets/709acd9e-0fc8-4210-b2b6-ade5f8f6b1b0" />

## v. Correctness Check (SIMD XMM)

<img width="520" height="327" alt="check_xmm" src="https://github.com/user-attachments/assets/aaa408c6-e08e-4f8c-83d3-10bc08e0bbb9" />

## vi. Correctness Check (SIMD YMM)

<img width="520" height="327" alt="check_ymm" src="https://github.com/user-attachments/assets/71a3fcef-d921-4b5d-ae69-9ba1286d1fc0" />

## vii. Boundary Check (SIMD XMM and YMM)

<img width="620" height="115" alt="boundary" src="https://github.com/user-attachments/assets/d428dd5b-cc28-432c-8a68-cbe20c345af3" />

The sizes 1, 3, 1000, 1001, 1002 and 1003 cover every leftover case: 0 or 1 element left for XMM, and 0 to 3 elements left for YMM. n = 1 and n = 3 also check the case where the YMM loop does not run at all because n is smaller than 4. All kernels matched C.

## viii. Analysis

**Which one is fastest?** YMM is the fastest at every size, followed by XMM, then x86-64, then Debug C. At 2³⁰, YMM is 4.11x faster than Debug C and 1.90x faster than x86-64.

**Why is it faster?** YMM processes 4 doubles per instruction and XMM processes 2, so both need far fewer loop iterations than the scalar version. This means fewer loads, multiplies, adds and loop overhead for the same amount of data.

**Why is the speedup lower than the theoretical 2x and 4x?** In theory XMM should be 2x faster and YMM 4x faster than scalar code. Instead, XMM reaches only 1.5–1.7x and YMM only 1.9–2.3x.

The main reason is that the kernel is **memory-bound**. Each element needs only one multiply and one add for every 8 bytes loaded, so the CPU can do the math faster than RAM can deliver the data. YMM reads at about 13–15 GB/s at every size, including 2²⁰. This suggests that even the 8 MB vector is not fully served from cache on this machine. Once the CPU is waiting on memory, processing more elements per instruction does not help much. That is why YMM is only 1.25–1.42x faster than XMM even though its registers are twice as wide. The gap is smallest at 2³⁰ (8 GB), where every element has to come from RAM.

Other reasons the speedup is below theory:

- Each `addsd` or `vaddpd` has to wait for the previous one to finish because they all add into the same register.
- The loop counter, compare and jump still run on every iteration.
- Laptop CPUs change clock speed with temperature and power, which adds noise to the timings.

**Debug vs Release:** The assembly kernels are the same in both builds, because Debug and Release only change how the C code is compiled. The main difference between the two runs is therefore the C kernel.

_(fill after Release run)_

**Is SIMD YMM faster than optimized C?**

_(fill after Release run)_

## ix. Problems Encountered and AHA Moments

- **Not enough RAM for 2³⁰.** 2³⁰ doubles needs 8 GB. Windows can let a large allocation succeed by using the page file on disk, which would make the timings meaningless. To avoid this, the program checks free physical RAM with `GlobalMemoryStatusEx` before allocating and falls back to 2²⁹, then 2²⁸. One laptop could only fit 2²⁸ or 2²⁹ depending on what else was open, so the final results were run on a groupmate's laptop that could fit 2³⁰.
- **SIMD results did not exactly match C.** The XMM and YMM results differed from C in the last few digits, with relative differences up to about 2e-13. This happens because SIMD keeps several partial sums and adds them at the end, and floating-point addition gives slightly different results in a different order. The fix was to compare with a small tolerance instead of `==`.
- **Boundary elements.** A vector size like 1003 is not a multiple of 4. Rounding n down with `and r8, -4` and adding a scalar loop for the leftovers handles any n without reading past the end of the array.
- **Timings vary between runs and machines.** The same program gave noticeably different times on the two laptops. For example, x86-64 at 2²⁰ took about 0.6 ms on one and 1.3 ms on the other. On the first laptop, XMM was even slightly faster than YMM at 2²⁶. This is why each kernel is averaged over 30 runs with a warm-up call, and why all results in this report come from one machine.
- **AHA: XMM registers for scalar math.** The scalar x86-64 kernel uses `xmm0` and `xmm1` even though it is not SIMD. x86-64 does floating-point math in XMM registers, and the instruction decides how much is used: `mulsd` (scalar double) uses only the low lane, while `mulpd` (packed double) uses all lanes. `xmm0` is also where Windows x64 expects a returned `double`.
- **AHA: Wider registers do not mean proportionally faster.** YMM registers are twice as wide as XMM, but YMM was only 1.25–1.42x faster. For a simple kernel like this, memory speed matters more than how many elements an instruction can process.
