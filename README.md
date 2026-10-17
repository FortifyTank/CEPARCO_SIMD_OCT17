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

- **Data:** `A[i] = sin(i * 0.001)`
- **Vector sizes:** n = 2²⁰, 2²⁶, 2²⁹. 2³⁰ (8 GB) did not fit in the laptop's free RAM, so the program fell back to 2²⁹ (4 GB).
- **Timing:** Only the kernel call is timed, using `QueryPerformanceCounter`. Each kernel is run once untimed as a warm-up, then the average of 30 runs is reported.
- **Correctness:** The C kernel is the answer key. Each assembly kernel is marked correct if its result is within a relative difference of 1e-9 of C.

## Boundary Handling

The SIMD loop only runs up to `n` rounded down to a multiple of 2 (XMM) or 4 (YMM). A scalar loop then processes the leftover elements. For example, with n = 1003 the YMM kernel processes 1000 elements 4 at a time and the last 3 one at a time.

---

# Report

## i. Program Output (Execution Time)

**Debug**

![Debug output](screenshots/debug_output.png)

**Release**

![Release output](screenshots/release_output.png)

## ii. Execution Time and Comparison

Average execution time in ms over 30 runs.

**Debug**

| n | C | x86-64 | XMM | YMM |
|---|---|---|---|---|
| 2²⁰ | 2.8814 | 0.6177 | 0.3168 | 0.2422 |
| 2²⁶ | 193.0350 | 42.2312 | 25.6345 | 28.2766 |
| 2²⁹ | 1576.4153 | 365.7957 | 234.7471 | 228.7997 |

**Release**

| n | C | x86-64 | XMM | YMM |
|---|---|---|---|---|
| 2²⁰ | | | | |
| 2²⁶ | | | | |
| 2²⁹ | | | | |

**Speedup over C (Debug)**

| n | x86-64 | XMM | YMM |
|---|---|---|---|
| 2²⁰ | 4.66x | 9.10x | 11.90x |
| 2²⁶ | 4.57x | 7.53x | 6.83x |
| 2²⁹ | 4.31x | 6.72x | 6.89x |

**Speedup over x86-64 (Debug)**

| n | Data size | XMM | YMM |
|---|---|---|---|
| 2²⁰ | 8 MB | 1.95x | 2.55x |
| 2²⁶ | 512 MB | 1.65x | 1.49x |
| 2²⁹ | 4 GB | 1.56x | 1.60x |

**Why the kernels differ:**

- **x86-64 vs Debug C:** Debug builds are not optimized, so the C loop stores `i` and `sum` to memory and reads them back on every iteration. The assembly version keeps everything in registers, which makes it about 4x faster.
- **XMM vs x86-64:** XMM squares and adds 2 doubles per instruction, so the loop runs half as many times.
- **YMM vs x86-64:** YMM handles 4 doubles per instruction, so the loop runs a quarter as many times. It is the fastest kernel on the small vector, but on the large vectors it is about the same speed as XMM (see viii).

## iii. Correctness Check (C)

![C output](screenshots/check_c.png)

## iv. Correctness Check (x86-64)

![x86-64 output](screenshots/check_asm.png)

## v. Correctness Check (SIMD XMM)

![XMM output](screenshots/check_xmm.png)

## vi. Correctness Check (SIMD YMM)

![YMM output](screenshots/check_ymm.png)

## vii. Boundary Check (SIMD XMM and YMM)

![Boundary check](screenshots/boundary.png)

The sizes 1, 3, 1000, 1001, 1002 and 1003 cover every leftover case: 0 or 1 element left for XMM, and 0 to 3 elements left for YMM. n = 1 and n = 3 also check the case where the YMM loop does not run at all because n is smaller than 4. All kernels matched C.

## viii. Analysis

**Which one is fastest?** Both SIMD kernels are faster than x86-64, and x86-64 is faster than Debug C. YMM is the fastest on the 2²⁰ vector. On the 2²⁶ and 2²⁹ vectors, XMM and YMM are about equally fast, and XMM was even slightly faster at 2²⁶.

**Why is the speedup lower than the theoretical 2x and 4x?** In theory XMM should be 2x faster and YMM 4x faster than scalar code. At 2²⁰, XMM comes close (1.95x) and YMM reaches 2.55x, because the 8 MB vector is small enough to be served mostly from the CPU cache. At 2²⁶ and 2²⁹, both drop to about 1.5–1.6x, and YMM is no better than XMM.

This is because the kernel is **memory-bound**. Each element needs only one multiply and one add for every 8 bytes loaded, so the CPU finishes the math faster than RAM can deliver the data. On the large vectors, XMM and YMM both read at about 18–21 GB/s, which is close to the memory bandwidth of the laptop. Both kernels hit the same limit, so processing 4 elements per instruction instead of 2 no longer helps.

Other reasons the speedup is below theory:

- Each `vaddpd` has to wait for the previous one to finish because they all add into the same register.
- The loop counter, compare and jump still run on every iteration.
- Laptop CPUs change clock speed with temperature and power, which adds noise to the timings.

**Debug vs Release:**

_(fill after Release run)_

**Is SIMD YMM faster than optimized C?**

_(fill after Release run)_

## ix. Problems Encountered and AHA Moments

- **Not enough RAM for 2³⁰.** 2³⁰ doubles needs 8 GB, and 2²⁹ needs 4 GB. Windows can let a large allocation succeed by using the page file on disk, which would make the timings meaningless. To avoid this, the program checks free physical RAM with `GlobalMemoryStatusEx` before allocating and falls back to 2²⁹, then 2²⁸. The amount of free RAM changed between runs depending on what else was open. One run could not fit even 2²⁸ (2 GB), while the final run fit 2²⁹ (4 GB).
- **SIMD results did not exactly match C.** The XMM and YMM results differed from C in the last few digits (relative difference up to about 2e-13). This happens because SIMD keeps several partial sums and adds them at the end, and floating-point addition gives slightly different results in a different order. The fix was to compare with a small tolerance instead of `==`.
- **Boundary elements.** A vector size like 1003 is not a multiple of 4. Rounding n down with `and r8, -4` and adding a scalar loop for the leftovers handles any n without reading past the end of the array.
- **AHA: XMM registers for scalar math.** The scalar x86-64 kernel uses `xmm0` and `xmm1` even though it is not SIMD. x86-64 does floating-point math in XMM registers, and the instruction decides how much is used: `mulsd` (scalar double) uses only the low lane, while `mulpd` (packed double) uses all lanes. `xmm0` is also where Windows x64 expects a returned `double`.
- **Timings vary between runs.** Running the same program again gave noticeably different times. In one earlier run, YMM was 3.5x faster than scalar at 2²⁰ instead of 2.55x. This is why each kernel is averaged over 30 runs with a warm-up call, but laptop clock speed changes with temperature and power still cause some variation.
- **AHA: Wider registers stop helping on big data.** At 2²⁶, XMM was actually faster than YMM. Once the data no longer fits in cache, memory speed matters more than how many elements an instruction can process.
