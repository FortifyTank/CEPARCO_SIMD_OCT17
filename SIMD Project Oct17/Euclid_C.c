#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>

#define RUNS 30

extern double norm_asm(double* A, long long n);
extern double norm_xmm(double* A, long long n);
extern double norm_ymm(double* A, long long n);

double norm_c(double* A, long long n) {
    double sum = 0.0;
    for (long long i = 0; i < n; i++)
        sum += A[i] * A[i];
    return sqrt(sum);
}

void init(double* A, long long n) {
    for (long long i = 0; i < n; i++)
        A[i] = sin(i * 0.001);
}

// SIMD adds in a different order so allow a tiny difference
int check(double ref, double x) {
    return fabs(ref - x) <= 1e-9 * fabs(ref);
}

// free RAM in MB
long long free_mb() {
    MEMORYSTATUSEX m;
    m.dwLength = sizeof(m);
    GlobalMemoryStatusEx(&m);
    return (long long)(m.ullAvailPhys / (1024 * 1024));
}

// average time in ms of RUNS calls, only the kernel is timed
double timeit(double (*f)(double*, long long), double* A, long long n, double* z) {
    LARGE_INTEGER freq, t1, t2;
    double total = 0;
    QueryPerformanceFrequency(&freq);

    *z = f(A, n); // warm up

    for (int r = 0; r < RUNS; r++) {
        QueryPerformanceCounter(&t1);
        *z = f(A, n);
        QueryPerformanceCounter(&t2);
        total += (double)(t2.QuadPart - t1.QuadPart) * 1000.0 / freq.QuadPart;
    }
    return total / RUNS;
}

void run(double* A, long long n) {
    double zc, za, zx, zy;
    double tc, ta, tx, ty;

    tc = timeit(norm_c, A, n, &zc);
    ta = timeit(norm_asm, A, n, &za);
    tx = timeit(norm_xmm, A, n, &zx);
    ty = timeit(norm_ymm, A, n, &zy);

    printf("\nn = %lld\n", n);
    printf("C   Z = %.10f  time = %.4f ms\n", zc, tc);
    printf("ASM Z = %.10f  time = %.4f ms  %s\n", za, ta, check(zc, za) ? "correct" : "WRONG");
    printf("XMM Z = %.10f  time = %.4f ms  %s\n", zx, tx, check(zc, zx) ? "correct" : "WRONG");
    printf("YMM Z = %.10f  time = %.4f ms  %s\n", zy, ty, check(zc, zy) ? "correct" : "WRONG");
}

int main() {
    // boundary test, sizes not divisible by 2 or 4
    long long b[6] = { 1, 3, 1000, 1001, 1002, 1003 };

    printf("boundary test\n");
    for (int i = 0; i < 6; i++) {
        long long n = b[i];
        double* A = malloc(n * sizeof(double));
        init(A, n);
        double zc = norm_c(A, n);
        double za = norm_asm(A, n);
        double zx = norm_xmm(A, n);
        double zy = norm_ymm(A, n);
        printf("n = %lld  C = %.10f  ASM %s  XMM %s  YMM %s\n", n, zc,
            check(zc, za) ? "correct" : "WRONG",
            check(zc, zx) ? "correct" : "WRONG",
            check(zc, zy) ? "correct" : "WRONG");
        free(A);
    }

    long long sizes[3] = { 1LL << 20, 1LL << 26, 1LL << 30 };
    int p[3] = { 20, 26, 30 };

    for (int s = 0; s < 3; s++) {
        long long n = sizes[s];

        // 2^30 can go down to 2^29 or 2^28 if not enough free RAM
        if (s == 2) {
            while (p[s] >= 28 && n * 8 / (1024 * 1024) >= free_mb()) {
                printf("\n2^%d needs %lld MB, only %lld MB free\n", p[s], n * 8 / (1024 * 1024), free_mb());
                n /= 2;
                p[s]--;
            }
            if (p[s] < 28) {
                printf("not enough RAM for 2^28\n");
                break;
            }
        }

        double* A = malloc(n * sizeof(double));
        if (A == NULL) {
            printf("malloc failed for 2^%d\n", p[s]);
            continue;
        }
        init(A, n);
        run(A, n);
        free(A);
    }
    return 0;
}
