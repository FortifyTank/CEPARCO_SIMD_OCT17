#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <windows.h>
#include <math.h>

#define RUNS 30   // each kernel is run this many times, we report the average

// all 4 kernels have the same signature:
// rcx = pointer to A, rdx = n, answer comes back in xmm0 (double)
extern double norm_asm(const double* A, long long n);
extern double norm_xmm(const double* A, long long n);
extern double norm_ymm(const double* A, long long n);

typedef double (*kernel_fn)(const double* A, long long n);

// Euclidean norm in C, this is our reference answer
// Z = sqrt(A[0]^2 + A[1]^2 + ... + A[n-1]^2)
double norm_c(const double* A, long long n) {
    double sum = 0.0;
    for (long long i = 0; i < n; i++)
        sum += A[i] * A[i];
    return sqrt(sum);
}

// data we chose: A[i] = sin(i * 0.001)
// (documented in the README too)
void init_array(double* A, long long n) {
    for (long long i = 0; i < n; i++)
        A[i] = sin(i * 0.001);
}

// floating point addition order is different in SIMD (partial sums),
// so the last digits can differ. we compare with a tiny relative tolerance
// instead of ==
int close_enough(double ref, double test) {
    return fabs(ref - test) <= 1e-9 * fabs(ref);
}

// runs a kernel RUNS times, times ONLY the kernel call,
// returns the average time in seconds and saves the last result
double time_kernel(kernel_fn f, const double* A, long long n, double* result) {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    double total = 0.0;

    for (int r = 0; r < RUNS; r++) {
        QueryPerformanceCounter(&start);
        *result = f(A, n);
        QueryPerformanceCounter(&end);
        total += (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    }
    return total / RUNS;
}

// full test for one vector size: time all 4, check all 3 asm against C
void run_size(const double* A, long long n) {
    double z_c, z_asm, z_xmm, z_ymm;
    double t_c, t_asm, t_xmm, t_ymm;

    printf("==== n = %lld ====\n", n);

    t_c   = time_kernel(norm_c,   A, n, &z_c);
    t_asm = time_kernel(norm_asm, A, n, &z_asm);
    t_xmm = time_kernel(norm_xmm, A, n, &z_xmm);
    t_ymm = time_kernel(norm_ymm, A, n, &z_ymm);

    printf("[C]   Z = %.12f   avg time: %.6f s\n", z_c, t_c);
    printf("[ASM] Z = %.12f   avg time: %.6f s   %s\n", z_asm, t_asm,
        close_enough(z_c, z_asm) ? "CORRECT" : "WRONG");
    printf("[XMM] Z = %.12f   avg time: %.6f s   %s\n", z_xmm, t_xmm,
        close_enough(z_c, z_xmm) ? "CORRECT" : "WRONG");
    printf("[YMM] Z = %.12f   avg time: %.6f s   %s\n", z_ymm, t_ymm,
        close_enough(z_c, z_ymm) ? "CORRECT" : "WRONG");

    // speedup vs C
    printf("Speedup vs C -> ASM: %.2fx  XMM: %.2fx  YMM: %.2fx\n\n",
        t_c / t_asm, t_c / t_xmm, t_c / t_ymm);
}

// step 3a: sizes that are NOT a multiple of 2 (XMM) or 4 (YMM)
// the leftover elements must be handled by the scalar tail loop
void boundary_test(void) {
    long long sizes[] = { 1, 3, 1000, 1001, 1002, 1003 };
    int count = sizeof(sizes) / sizeof(sizes[0]);

    printf("######## BOUNDARY CHECK ########\n");
    for (int s = 0; s < count; s++) {
        long long n = sizes[s];
        double* A = (double*)_aligned_malloc((size_t)n * sizeof(double), 32);
        if (A == NULL) { printf("error.\n"); return; }
        init_array(A, n);

        double z_c   = norm_c(A, n);
        double z_asm = norm_asm(A, n);
        double z_xmm = norm_xmm(A, n);
        double z_ymm = norm_ymm(A, n);

        printf("n = %-5lld (n mod 2 = %lld, n mod 4 = %lld)\n", n, n % 2, n % 4);
        printf("  C   : %.12f\n", z_c);
        printf("  ASM : %.12f  %s\n", z_asm, close_enough(z_c, z_asm) ? "CORRECT" : "WRONG");
        printf("  XMM : %.12f  %s\n", z_xmm, close_enough(z_c, z_xmm) ? "CORRECT" : "WRONG");
        printf("  YMM : %.12f  %s\n", z_ymm, close_enough(z_c, z_ymm) ? "CORRECT" : "WRONG");

        _aligned_free(A);
    }
    printf("\n");
}

int main() {
    boundary_test();

    // 2^20 and 2^26 are required. 2^30 doubles = 8 GB, so if the
    // allocation fails we fall back to 2^29 then 2^28
    int shifts[] = { 20, 26, 30 };

    for (int s = 0; s < 3; s++) {
        int shift = shifts[s];
        double* A = NULL;

        // only 2^30 is allowed to fall back (down to 2^28)
        int min_shift = (shifts[s] == 30) ? 28 : shifts[s];

        for (; shift >= min_shift; shift--) {
            long long n = 1LL << shift;
            A = (double*)_aligned_malloc((size_t)n * sizeof(double), 32);
            if (A != NULL) break;
            printf("2^%d too big for this machine, trying smaller...\n", shift);
        }

        if (A == NULL) {
            printf("error: could not allocate for 2^%d\n", shifts[s]);
            continue;
        }

        long long n = 1LL << shift;
        printf("(initializing 2^%d elements...)\n", shift);
        init_array(A, n);
        run_size(A, n);
        _aligned_free(A);
    }
    return 0;
}
