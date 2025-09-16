#include "src/libgpufw.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if(argc != 3) {
        printf("Usage: %s <kernel_file> <vector_size>\n", argv[0]);
        return -1;
    }

    const char *kernel_file = argv[1];
    int n = atoi(argv[2]);

    gpufw_ctx ctx;
    if(gpufw_init_from_file(&ctx, kernel_file, 0) != 0) {
        printf("GPU init failed\n");
        return -1;
    }

    size_t bytes = n * sizeof(float);
    float *a = (float*)malloc(bytes);
    float *b = (float*)malloc(bytes);
    float *c = (float*)malloc(bytes);

    for(int i = 0; i < n; i++) { a[i] = i; b[i] = n - i; }

    cl_kernel kernel = clCreateKernel(ctx.program, "vecadd", NULL);
    cl_mem buf_a = gpufw_alloc_buffer(&ctx, bytes, CL_MEM_READ_ONLY);
    cl_mem buf_b = gpufw_alloc_buffer(&ctx, bytes, CL_MEM_READ_ONLY);
    cl_mem buf_c = gpufw_alloc_buffer(&ctx, bytes, CL_MEM_WRITE_ONLY);

    gpufw_write_buffer(&ctx, buf_a, a, bytes);
    gpufw_write_buffer(&ctx, buf_b, b, bytes);

    gpufw_set_kernel_arg(&ctx, kernel, 0, sizeof(cl_mem), &buf_a);
    gpufw_set_kernel_arg(&ctx, kernel, 1, sizeof(cl_mem), &buf_b);
    gpufw_set_kernel_arg(&ctx, kernel, 2, sizeof(cl_mem), &buf_c);
    gpufw_set_kernel_arg(&ctx, kernel, 3, sizeof(int), &n);

    gpufw_launch_kernel(&ctx, kernel, n, 64);

    gpufw_read_buffer(&ctx, buf_c, c, bytes);

    for(int i = 0; i < 10; i++)
        printf("%d + %d = %f\n", i, n-i, c[i]);

    clReleaseMemObject(buf_a);
    clReleaseMemObject(buf_b);
    clReleaseMemObject(buf_c);
    clReleaseKernel(kernel);
    gpufw_cleanup(&ctx);

    free(a); free(b); free(c);
    return 0;
}