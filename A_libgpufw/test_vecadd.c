#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include "src/libgpufw.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <kernel_file> <vector_size>\n", argv[0]);
        return 1;
    }

    const char *kernel_path = argv[1];
    int n = atoi(argv[2]);

    gpufw_ctx ctx;
    if (gpufw_init_from_file(&ctx, kernel_path, "vecadd", 0) != 0) {
        fprintf(stderr, "gpufw_init failed\n");
        return 1;
    }

    // Allocate host memory
    float *A = (float *)malloc(sizeof(float) * n);
    float *B = (float *)malloc(sizeof(float) * n);
    float *C = (float *)malloc(sizeof(float) * n);

    for (int i = 0; i < n; i++) {
        A[i] = i * 1.0f;
        B[i] = (n - i) * 0.5f;
    }

    // Create buffers
    cl_int err;
    cl_mem bufA = clCreateBuffer(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                 sizeof(float) * n, A, &err);
    cl_mem bufB = clCreateBuffer(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                 sizeof(float) * n, B, &err);
    cl_mem bufC = clCreateBuffer(ctx.context, CL_MEM_WRITE_ONLY,
                                 sizeof(float) * n, NULL, &err);

    // Create kernel
    cl_kernel kernel = clCreateKernel(ctx.program, "vecadd", &err);

    // Set kernel args
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufC);
    clSetKernelArg(kernel, 3, sizeof(unsigned int), &n);

    // Execute kernel
    size_t global_work_size = n;
    err = clEnqueueNDRangeKernel(ctx.queue, kernel, 1, NULL,
                                 &global_work_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error launching kernel (%d)\n", err);
        return 1;
    }

    // Read result back
    clEnqueueReadBuffer(ctx.queue, bufC, CL_TRUE, 0,
                        sizeof(float) * n, C, 0, NULL, NULL);

    // Verify (check first 10 results)
    printf("Sample results:\n");
    for (int i = 0; i < 10; i++) {
        printf("C[%d] = %f (expected %f)\n", i, C[i], A[i] + B[i]);
    }

    // Cleanup
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufC);
    clReleaseKernel(kernel);

    free(A);
    free(B);
    free(C);

    gpufw_cleanup(&ctx);
    return 0;
}