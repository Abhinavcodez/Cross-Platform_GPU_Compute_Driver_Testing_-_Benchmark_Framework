#include "libgpufw.h"
#include <stdio.h>
#include <stdlib.h>

// Helper: read kernel source file
static char* read_kernel_source(const char *filename, size_t *length) {
    FILE *f = fopen(filename, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END);
    *length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *source = (char*)malloc(*length + 1);
    fread(source, 1, *length, f);
    source[*length] = '\0';
    fclose(f);
    return source;
}

// Initialize OpenCL context from kernel file
int gpufw_init_from_file(gpufw_ctx *ctx, const char *kernel_file, int device_index) {
    cl_int err;
    cl_uint num_platforms;
    if(clGetPlatformIDs(0, NULL, &num_platforms) != CL_SUCCESS || num_platforms == 0) {
        printf("No OpenCL platforms found!\n");
        return -1;
    }
    cl_platform_id *platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
    clGetPlatformIDs(num_platforms, platforms, NULL);
    ctx->platform = platforms[0]; // pick first platform
    free(platforms);

    cl_uint num_devices;
    if(clGetDeviceIDs(ctx->platform, CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices) != CL_SUCCESS || num_devices == 0) {
        printf("No GPU device found, falling back to CPU\n");
        clGetDeviceIDs(ctx->platform, CL_DEVICE_TYPE_CPU, 1, &ctx->device, NULL);
    } else {
        cl_device_id *devices = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices);
        clGetDeviceIDs(ctx->platform, CL_DEVICE_TYPE_GPU, num_devices, devices, NULL);
        ctx->device = devices[device_index % num_devices];
        free(devices);
    }

    ctx->context = clCreateContext(NULL, 1, &ctx->device, NULL, NULL, &err);
    if(err != CL_SUCCESS) { printf("clCreateContext failed\n"); return -1; }
    ctx->queue = clCreateCommandQueueWithProperties(ctx->context, ctx->device, 0, &err);
    if(err != CL_SUCCESS) { printf("clCreateCommandQueue failed\n"); return -1; }

    size_t src_size;
    char *src = read_kernel_source(kernel_file, &src_size);
    if(!src) { printf("Failed to read kernel file\n"); return -1; }

    ctx->program = clCreateProgramWithSource(ctx->context, 1, (const char**)&src, &src_size, &err);
    free(src);
    if(err != CL_SUCCESS) { printf("clCreateProgramWithSource failed\n"); return -1; }

    err = clBuildProgram(ctx->program, 1, &ctx->device, NULL, NULL, NULL);
    if(err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(ctx->program, ctx->device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char*)malloc(log_size);
        clGetProgramBuildInfo(ctx->program, ctx->device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build log:\n%s\n", log);
        free(log);
        return -1;
    }

    return 0;
}

// Buffer management
cl_mem gpufw_alloc_buffer(gpufw_ctx *ctx, size_t size, cl_mem_flags flags) {
    cl_int err;
    cl_mem buf = clCreateBuffer(ctx->context, flags, size, NULL, &err);
    if(err != CL_SUCCESS) { printf("Buffer allocation failed\n"); return NULL; }
    return buf;
}

int gpufw_write_buffer(gpufw_ctx *ctx, cl_mem buf, const void *host_ptr, size_t size) {
    return clEnqueueWriteBuffer(ctx->queue, buf, CL_TRUE, 0, size, host_ptr, 0, NULL, NULL);
}

int gpufw_read_buffer(gpufw_ctx *ctx, cl_mem buf, void *host_ptr, size_t size) {
    return clEnqueueReadBuffer(ctx->queue, buf, CL_TRUE, 0, size, host_ptr, 0, NULL, NULL);
}

// Kernel argument
int gpufw_set_kernel_arg(gpufw_ctx *ctx, cl_kernel kernel, int index, size_t size, const void *value) {
    return clSetKernelArg(kernel, index, size, value);
}

// Launch kernel
int gpufw_launch_kernel(gpufw_ctx *ctx, cl_kernel kernel, size_t global_work_size, size_t local_work_size) {
    size_t gws[1] = { global_work_size };
    size_t lws[1] = { local_work_size };
    return clEnqueueNDRangeKernel(ctx->queue, kernel, 1, NULL, gws, lws, 0, NULL, NULL);
}

// Cleanup
void gpufw_cleanup(gpufw_ctx *ctx) {
    if(ctx->program) clReleaseProgram(ctx->program);
    if(ctx->queue) clReleaseCommandQueue(ctx->queue);
    if(ctx->context) clReleaseContext(ctx->context);
}