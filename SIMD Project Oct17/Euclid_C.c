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
void init_array(double* A, long long n) {
    for (long long i = 0; i < n; i++)
        A[i] = sin(i * 0.001);
}

double rel_error(double ref, double test) {
    if (ref == 0.0) return fabs(test);
    return fabs(ref - test) / fabs(ref);
}

// SIMD adds in a different order (partial sums), so the last digits
// can differ. compare with a small relative tolerance instead of ==
int close_enough(double ref, double test) {
    return rel_error(ref, test) <= 1e-9;
}

// only allocate if the array fits in free physical RAM,
// otherwise Windows uses the page file and the timing is meaningless
int fits_in_ram(long long n) {
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    if (!GlobalMemoryStatusEx(&mem)) return 1;
    return (unsigned long long)n * sizeof(double) < mem.ullAvailPhys;
}

// one untimed warm-up call, then times ONLY the kernel call RUNS times
// returns the average time in milliseconds and saves the last result
double time_kernel(kernel_fn f, const double* A, long long n, double* result) {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    double total = 0.0;

    *result = f(A, n);  // warm-up

    for (int r = 0; r < RUNS; r++) {
        QueryPerformanceCounter(&start);
        *result = f(A, n);
        QueryPerformanceCounter(&end);
        total += (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    }
    return total * 1000.0 / RUNS;
}

void print_check(const char* name, double z_ref, double z, double t_ms) {
    printf("[%s] Z = %.12f   avg time: %10.4f ms   rel err: %.2e   %s\n",
        name, z, t_ms, rel_error(z_ref, z),
        close_enough(z_ref, z) ? "CORRECT" : "WRONG");
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

    printf("[C]   Z = %.12f   avg time: %10.4f ms\n", z_c, t_c);
    print_check("ASM", z_c, z_asm, t_asm);
    print_check("XMM", z_c, z_xmm, t_xmm);
    print_check("YMM", z_c, z_ymm, t_ymm);

    printf("Speedup vs C -> ASM: %.2fx  XMM: %.2fx  YMM: %.2fx\n\n",
        t_c / t_asm, t_c / t_xmm, t_c / t_ymm);
}

// sizes that are NOT a multiple of 2 (XMM) or 4 (YMM)
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
        printf("  ASM : %.12f  rel err: %.2e  %s\n", z_asm, rel_error(z_c, z_asm),
            close_enough(z_c, z_asm) ? "CORRECT" : "WRONG");
        printf("  XMM : %.12f  rel err: %.2e  %s\n", z_xmm, rel_error(z_c, z_xmm),
            close_enough(z_c, z_xmm) ? "CORRECT" : "WRONG");
        printf("  YMM : %.12f  rel err: %.2e  %s\n", z_ymm, rel_error(z_c, z_ymm),
            close_enough(z_c, z_ymm) ? "CORRECT" : "WRONG");

        _aligned_free(A);
    }
    printf("\n");
}

int main() {
    boundary_test();

    // 2^20 and 2^26 are required. 2^30 doubles = 8 GB, so if it does not
    // fit in free RAM we fall back to 2^29 then 2^28
    int shifts[] = { 20, 26, 30 };

    for (int s = 0; s < 3; s++) {
        int shift = shifts[s];
        double* A = NULL;

        // only 2^30 is allowed to fall back (down to 2^28)
        int min_shift = (shifts[s] == 30) ? 28 : shifts[s];

        for (; shift >= min_shift; shift--) {
            long long n = 1LL << shift;
            if (fits_in_ram(n)) {
                A = (double*)_aligned_malloc((size_t)n * sizeof(double), 32);
                if (A != NULL) break;
            }
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
