#include <stdio.h>
#include <stdlib.h>
#include "libgpufw.h"

// Embedded OpenCL kernel (no file I/O needed)
static const char *vecadd_kernel_src =
"__kernel void vecadd(__global const float *a, __global const float *b, __global float *c, int n) {\n"
"    int gid = get_global_id(0);\n"
"    if (gid < n) c[gid] = a[gid] + b[gid];\n"
"}\n";

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <vector_size>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    size_t bytes = n * sizeof(float);

    // Allocate host arrays
    float *a = (float*)malloc(bytes);
    float *b = (float*)malloc(bytes);
    float *c = (float*)malloc(bytes);
    for (int i = 0; i < n; i++) {
        a[i] = (float)i;
        b[i] = (float)(n - i);
    }

    // GPU context
    gpufw_ctx ctx;
    if (gpufw_init_from_string(&ctx, vecadd_kernel_src, "vecadd", 0) != 0) {
        fprintf(stderr, "Failed to init GPU context\n");
        return 1;
    }

    // Create buffers
    cl_mem buf_a = gpufw_alloc_buffer(&ctx, bytes, CL_MEM_READ_ONLY);
    cl_mem buf_b = gpufw_alloc_buffer(&ctx, bytes, CL_MEM_READ_ONLY);
    cl_mem buf_c = gpufw_alloc_buffer(&ctx, bytes, CL_MEM_WRITE_ONLY);

    // Transfer data
    gpufw_write_buffer(&ctx, buf_a, a, bytes);
    gpufw_write_buffer(&ctx, buf_b, b, bytes);

    // Set kernel args
    gpufw_set_kernel_arg(&ctx, 0, sizeof(cl_mem), &buf_a);
    gpufw_set_kernel_arg(&ctx, 1, sizeof(cl_mem), &buf_b);
    gpufw_set_kernel_arg(&ctx, 2, sizeof(cl_mem), &buf_c);
    gpufw_set_kernel_arg(&ctx, 3, sizeof(int), &n);

    // Launch kernel
    size_t global_work_size = n;
    gpufw_launch_kernel(&ctx, global_work_size, 0);

    // Read result back
    gpufw_read_buffer(&ctx, buf_c, c, bytes);

    // Verify
    for (int i = 0; i < 10; i++)
        printf("%f + %f = %f\n", a[i], b[i], c[i]);

    // Cleanup
    gpufw_free_buffer(buf_a);
    gpufw_free_buffer(buf_b);
    gpufw_free_buffer(buf_c);
    gpufw_cleanup(&ctx);

    free(a);
    free(b);
    free(c);
    return 0;
}