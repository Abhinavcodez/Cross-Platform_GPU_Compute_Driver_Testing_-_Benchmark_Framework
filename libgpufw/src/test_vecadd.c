// test_vecadd.c
#include "gpufw.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int main(int argc, char **argv) {
    int n = 1<<20; // default size
    if (argc>1) n = atoi(argv[1]);
    float *a = malloc(sizeof(float)*n);
    float *b = malloc(sizeof(float)*n);
    float *c = malloc(sizeof(float)*n);
    for(int i=0;i<n;i++){ a[i]=i*1.0f; b[i]=2.0f*i; }

    if (gpufw_init(0) != 0) { fprintf(stderr,"gpufw_init failed\n"); return 1; }
    gpufw_profile_t p={0};
    gpufw_run_vecadd(a,b,c,n,&p);
    printf("Kernel time: %.3f ms\n", p.kernel_ms);
    // quick correctness check
    for(int i=0;i<10;i++){
        printf("%f ", c[i]);
    }
    printf("\n");
    gpufw_cleanup();
    free(a); free(b); free(c);
    return 0;
}