// test_vecadd.c
#include "gpufw_opencl.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char **argv) {
    int n = 1<<20;
    if (argc > 1) n = atoi(argv[1]);
    float *a = malloc(sizeof(float) * (size_t)n);
    float *b = malloc(sizeof(float) * (size_t)n);
    float *c = malloc(sizeof(float) * (size_t)n);
    if (!a || !b || !c) { fprintf(stderr, "alloc fail\n"); return 1; }
    for (int i = 0; i < n; ++i) { a[i] = (float)i; b[i] = (float)(2*i); }

    if (gpufw_init(0) != 0) { fprintf(stderr, "gpufw_init failed\n"); return 1; }
    gpufw_profile_t prof = {0};
    if (gpufw_run_vecadd(a, b, c, n, &prof) != 0) { fprintf(stderr, "vecadd failed\n"); gpufw_cleanup(); return 1; }

    printf("Kernel time: %.3f ms\n", prof.kernel_ms);
    // quick correctness check
    for (int i = 0; i < 8; ++i) printf("%g ", c[i]);
    printf("\n");
    gpufw_cleanup();
    free(a); free(b); free(c);
    return 0;
}