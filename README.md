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

## Build and Run

- **Platform:** Windows, Visual Studio 2026, x64
- MASM must be enabled (*Build Dependencies → Build Customizations → masm*).
- Run in both Debug and Release with Ctrl+F5.

## Test Setup

- **Data:** `A[i] = sin(i * 0.001)`
- **Vector sizes:** n = 2²⁰, 2²⁶, 2³⁰. If 2³⁰ does not fit in free RAM, the program falls back to 2²⁹, then 2²⁸.
- **Timing:** Only the kernel call is timed. Each kernel is run once untimed as a warm-up, then the average of 30 runs is reported.
- **Correctness:** Each assembly kernel's output is checked against the C kernel with a relative tolerance of 1e-9. Exact `==` is not used because SIMD adds the numbers in a different order.

## Boundary Handling

The SIMD loop only runs up to `n` rounded down to a multiple of 2 (XMM) or 4 (YMM). A scalar loop then processes the leftover elements. This was tested with n = 1, 3, 1000, 1001, 1002 and 1003, and all kernels matched C.

## Results

Average execution time in ms over 30 runs. 2³⁰ (8 GB) and 2²⁹ (4 GB) did not fit in the laptop's free RAM, so the largest size is 2²⁸ (2 GB).

**Debug**

| n | C | x86-64 | XMM | YMM |
|---|---|---|---|---|
| 2²⁰ | 3.5395 | 0.7513 | 0.3552 | 0.2121 |
| 2²⁶ | 203.8247 | 50.7025 | 34.0457 | 26.4051 |
| 2²⁸ | 696.7100 | 167.2870 | 133.8770 | 119.0675 |

**Release**

| n | C | x86-64 | XMM | YMM |
|---|---|---|---|---|
| 2²⁰ | | | | |
| 2²⁶ | | | | |
| 2²⁸ | | | | |

## Analysis

**Memory-bound or compute-bound?** Memory-bound for large vectors. Each element needs only one multiply and one add for every 8 bytes loaded, so the CPU can do the math faster than RAM can deliver the data.

The SIMD speedup over scalar x86-64 shrinks as n grows:

| n | Data size | YMM vs x86-64 | XMM vs x86-64 | YMM vs XMM |
|---|---|---|---|---|
| 2²⁰ | 8 MB | 3.54x | 2.12x | 1.67x |
| 2²⁶ | 512 MB | 1.92x | 1.49x | 1.29x |
| 2²⁸ | 2 GB | 1.40x | 1.25x | 1.12x |

At 2²⁰ most of the data fits in cache, so the kernels are limited by computation. YMM comes close to the ideal 4x over scalar, and XMM comes close to the ideal 2x. At 2²⁶ and 2²⁸ the data has to come from RAM. YMM reads at only about 18–20 GB/s, which is around the memory bandwidth of a laptop. Wider registers cannot help once the CPU is waiting on memory, so YMM is barely ahead of XMM at 2²⁸.

In Debug, the C kernel is about 4x slower than scalar x86-64 at every size because Debug builds do not optimize. The loop variable and the sum are stored to memory and read back on every iteration.

**Is SIMD YMM faster than optimized C?**
