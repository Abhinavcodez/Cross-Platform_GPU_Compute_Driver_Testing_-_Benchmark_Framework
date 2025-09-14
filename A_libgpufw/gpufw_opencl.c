// gpufw.c
#define _POSIX_C_SOURCE 200809L
#include "gpufw_opencl.h"
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static cl_platform_id platform = NULL;
static cl_device_id device = NULL;
static cl_context ctx = NULL;
static cl_command_queue cq = NULL;
static cl_program program = NULL;
static cl_kernel vecadd_kernel = NULL;

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

int gpufw_init(int device_index) {
    cl_int err;
    cl_uint nplat = 0;
    if (clGetPlatformIDs(0, NULL, &nplat) != CL_SUCCESS || nplat == 0) {
        fprintf(stderr, "gpufw_init: no OpenCL platform found\n");
        return -1;
    }
    cl_platform_id *plats = malloc(sizeof(cl_platform_id) * nplat);
    clGetPlatformIDs(nplat, plats, NULL);
    platform = plats[0]; // choose first platform for now
    free(plats);

    cl_uint ndev = 0;
    if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &ndev) != CL_SUCCESS || ndev == 0) {
        fprintf(stderr, "gpufw_init: no OpenCL device found\n");
        return -1;
    }
    cl_device_id *devs = malloc(sizeof(cl_device_id) * ndev);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, ndev, devs, NULL);
    device = devs[(device_index >= 0 && device_index < (int)ndev) ? device_index : 0];
    free(devs);

    ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!ctx || err != CL_SUCCESS) { fprintf(stderr, "gpufw_init: clCreateContext failed (%d)\n", err); return -1; }

    cq = clCreateCommandQueueWithProperties(ctx, device, 0, &err);
    if (!cq || err != CL_SUCCESS) { fprintf(stderr, "gpufw_init: clCreateCommandQueue failed (%d)\n", err); clReleaseContext(ctx); return -1; }

    char *src = read_file("../kernels/vecadd.cl");
    if (!src) { fprintf(stderr, "gpufw_init: failed to read kernel source\n"); clReleaseCommandQueue(cq); clReleaseContext(ctx); return -1; }

    program = clCreateProgramWithSource(ctx, 1, (const char **)&src, NULL, &err);
    free(src);
    if (!program || err != CL_SUCCESS) { fprintf(stderr, "gpufw_init: clCreateProgramWithSource failed (%d)\n", err); gpufw_cleanup(); return -1; }

    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t loglen = 0;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &loglen);
        if (loglen) {
            char *log = malloc(loglen + 1);
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, loglen, log, NULL);
            log[loglen] = '\0';
            fprintf(stderr, "gpufw_init: build log:\n%s\n", log);
            free(log);
        } else {
            fprintf(stderr, "gpufw_init: clBuildProgram failed (%d)\n", err);
        }
        gpufw_cleanup();
        return -1;
    }

    vecadd_kernel = clCreateKernel(program, "vecadd", &err);
    if (!vecadd_kernel || err != CL_SUCCESS) { fprintf(stderr, "gpufw_init: clCreateKernel failed (%d)\n", err); gpufw_cleanup(); return -1; }

    return 0;
}

int gpufw_run_vecadd(const float *a, const float *b, float *c, int n, gpufw_profile_t *prof) {
    if (!ctx || !cq || !vecadd_kernel) return -1;
    cl_int err;
    size_t bytes = sizeof(float) * (size_t)n;

    cl_mem buf_a = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bytes, (void *)a, &err);
    if (err != CL_SUCCESS) { fprintf(stderr, "gpufw_run_vecadd: clCreateBuffer a failed (%d)\n", err); return -1; }
    cl_mem buf_b = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bytes, (void *)b, &err);
    if (err != CL_SUCCESS) { clReleaseMemObject(buf_a); fprintf(stderr, "gpufw_run_vecadd: clCreateBuffer b failed (%d)\n", err); return -1; }
    cl_mem buf_c = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, bytes, NULL, &err);
    if (err != CL_SUCCESS) { clReleaseMemObject(buf_a); clReleaseMemObject(buf_b); fprintf(stderr, "gpufw_run_vecadd: clCreateBuffer c failed (%d)\n", err); return -1; }

    err  = clSetKernelArg(vecadd_kernel, 0, sizeof(cl_mem), &buf_a);
    err |= clSetKernelArg(vecadd_kernel, 1, sizeof(cl_mem), &buf_b);
    err |= clSetKernelArg(vecadd_kernel, 2, sizeof(cl_mem), &buf_c);
    err |= clSetKernelArg(vecadd_kernel, 3, sizeof(int), &n);
    if (err != CL_SUCCESS) { clReleaseMemObject(buf_a); clReleaseMemObject(buf_b); clReleaseMemObject(buf_c); fprintf(stderr, "gpufw_run_vecadd: clSetKernelArg failed (%d)\n", err); return -1; }

    size_t global = (size_t)((n + 63) & ~63); // round up to warp/local-friendly size
    cl_event evt;
    err = clEnqueueNDRangeKernel(cq, vecadd_kernel, 1, NULL, &global, NULL, 0, NULL, &evt);
    if (err != CL_SUCCESS) { clReleaseMemObject(buf_a); clReleaseMemObject(buf_b); clReleaseMemObject(buf_c); fprintf(stderr, "gpufw_run_vecadd: clEnqueueNDRangeKernel failed (%d)\n", err); return -1; }

    // wait and profile
    clFinish(cq);
    if (prof) {
        cl_ulong start = 0, end = 0;
        if (clGetEventProfilingInfo(evt, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL) == CL_SUCCESS &&
            clGetEventProfilingInfo(evt, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL) == CL_SUCCESS &&
            end > start) {
            prof->kernel_ms = (double)(end - start) * 1e-6;
        } else {
            prof->kernel_ms = 0.0;
        }
    }

    err = clEnqueueReadBuffer(cq, buf_c, CL_TRUE, 0, bytes, c, 0, NULL, NULL);
    clReleaseEvent(evt);
    clReleaseMemObject(buf_a);
    clReleaseMemObject(buf_b);
    clReleaseMemObject(buf_c);

    if (err != CL_SUCCESS) { fprintf(stderr, "gpufw_run_vecadd: clEnqueueReadBuffer failed (%d)\n", err); return -1; }
    return 0;
}

void gpufw_cleanup(void) {
    if (vecadd_kernel) { clReleaseKernel(vecadd_kernel); vecadd_kernel = NULL; }
    if (program) { clReleaseProgram(program); program = NULL; }
    if (cq) { clReleaseCommandQueue(cq); cq = NULL; }
    if (ctx) { clReleaseContext(ctx); ctx = NULL; }
}