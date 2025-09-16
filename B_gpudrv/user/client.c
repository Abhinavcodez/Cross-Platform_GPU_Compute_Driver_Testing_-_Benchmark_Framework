#include <stdio.h>
#include <string.h>
#include "../include/gpudrv_ioctl.h"

// From libgpudrv
int gpudrv_open(void);
void gpudrv_close(void);
int gpudrv_run_cpu(void);
int gpudrv_run_gpu(void);
int gpudrv_run_hybrid(void);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [cpu|gpu|hybrid]\n", argv[0]);
        return 1;
    }

    if (gpudrv_open() < 0) return 1;

    if (strcmp(argv[1], "cpu") == 0) {
        gpudrv_run_cpu();
    } else if (strcmp(argv[1], "gpu") == 0) {
        gpudrv_run_gpu();
    } else if (strcmp(argv[1], "hybrid") == 0) {
        gpudrv_run_hybrid();
    } else {
        printf("Unknown mode: %s\n", argv[1]);
    }

    gpudrv_close();
    return 0;
}