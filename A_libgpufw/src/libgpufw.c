// corrected_libgpufw.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <CL/cl.h>
#include "libgpufw.h"   // must define gpufw_ctx struct (platform, device, context, queue, program, kernel optional)

static char* read_kernel_source(const char *filename, size_t *length) {
    if (!filename || !length) return NULL;
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "read_kernel_source: fopen('%s') failed: %s\n", filename, strerror(errno));
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);

    char *source = (char*)malloc((size_t)sz + 1);
    if (!source) { fclose(f); return NULL; }
    size_t got = fread(source, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { free(source); return NULL; }
    source[got] = '\0';
    *length = got;
    return source;
}

/* Prefer GPU across all platforms; if none, fall back to CPU.
   device_index selects among multiple devices of chosen type. */
int gpufw_init_from_file(gpufw_ctx *ctx, const char *kernel_file, int device_index) {
    if (!ctx || !kernel_file) return -1;
    cl_int err;
    cl_uint num_platforms = 0;
    err = clGetPlatformIDs(0, NULL, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        fprintf(stderr, "gpufw_init: clGetPlatformIDs found none (err=%d)\n", err);
        return -1;
    }

    cl_platform_id *platforms = malloc(sizeof(cl_platform_id) * num_platforms);
    if (!platforms) return -1;
    err = clGetPlatformIDs(num_platforms, platforms, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "gpufw_init: clGetPlatformIDs failed (%d)\n", err);
        free(platforms);
        return -1;
    }

    cl_device_id chosen_device = NULL;
    cl_platform_id chosen_platform = NULL;
    cl_uint dev_count = 0;

    /* 1) Try to find any GPU on any platform */
    for (cl_uint i = 0; i < num_platforms && !chosen_device; ++i) {
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 0, NULL, &dev_count);
        if (err == CL_SUCCESS && dev_count > 0) {
            cl_device_id *devs = malloc(sizeof(cl_device_id) * dev_count);
            if (!devs) continue;
            err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, dev_count, devs, NULL);
            if (err == CL_SUCCESS) {
                chosen_device = devs[(device_index >= 0) ? (device_index % dev_count) : 0];
                chosen_platform = platforms[i];
            }
            free(devs);
            break;
        }
    }

    /* 2) If no GPU found, try CPU on any platform */
    if (!chosen_device) {
        for (cl_uint i = 0; i < num_platforms && !chosen_device; ++i) {
            err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_CPU, 0, NULL, &dev_count);
            if (err == CL_SUCCESS && dev_count > 0) {
                cl_device_id *devs = malloc(sizeof(cl_device_id) * dev_count);
                if (!devs) continue;
                err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_CPU, dev_count, devs, NULL);
                if (err == CL_SUCCESS) {
                    chosen_device = devs[(device_index >= 0) ? (device_index % dev_count) : 0];
                    chosen_platform = platforms[i];
                }
                free(devs);
                break;
            }
        }
    }

    if (!chosen_device) {
        fprintf(stderr, "gpufw_init: no suitable OpenCL device found (gpu or cpu)\n");
        free(platforms);
        return -1;
    }

    /* fill ctx fields */
    ctx->platform = chosen_platform;
    ctx->device   = chosen_device;

    /* Create context */
    ctx->context = clCreateContext(NULL, 1, &ctx->device, NULL, NULL, &err);
    if (err != CL_SUCCESS || ctx->context == NULL) {
        fprintf(stderr, "gpufw_init: clCreateContext failed (%d)\n", err);
        free(platforms);
        return -1;
    }

    /* Create command queue (no special properties). If profiling required, set CL_QUEUE_PROFILING_ENABLE via properties. */
    /* Note: clCreateCommandQueueWithProperties returns error code in err. */
    //cl_command_queue_properties props[] = { 0 }; // placeholder, use NULL in call
    ctx->queue = clCreateCommandQueueWithProperties(ctx->context, ctx->device, NULL, &err);
    if (err != CL_SUCCESS || ctx->queue == NULL) {
        fprintf(stderr, "gpufw_init: clCreateCommandQueueWithProperties failed (%d)\n", err);
        clReleaseContext(ctx->context);
        ctx->context = NULL;
        free(platforms);
        return -1;
    }

    /* Read kernel source */
    size_t src_size = 0;
    char *src = read_kernel_source(kernel_file, &src_size);
    if (!src) {
        fprintf(stderr, "gpufw_init: Failed to read kernel file '%s'\n", kernel_file);
        clReleaseCommandQueue(ctx->queue);
        clReleaseContext(ctx->context);
        ctx->queue = NULL; ctx->context = NULL;
        free(platforms);
        return -1;
    }

    /* Create & build program */
    ctx->program = clCreateProgramWithSource(ctx->context, 1, (const char**)&src, &src_size, &err);
    free(src);
    if (err != CL_SUCCESS || ctx->program == NULL) {
        fprintf(stderr, "gpufw_init: clCreateProgramWithSource failed (%d)\n", err);
        clReleaseCommandQueue(ctx->queue);
        clReleaseContext(ctx->context);
        ctx->queue = NULL; ctx->context = NULL;
        free(platforms);
        return -1;
    }

    err = clBuildProgram(ctx->program, 1, &ctx->device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        /* retrieve build log */
        size_t log_size = 0;
        clGetProgramBuildInfo(ctx->program, ctx->device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        if (log_size > 0) {
            char *log = malloc(log_size + 1);
            if (log) {
                clGetProgramBuildInfo(ctx->program, ctx->device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
                log[log_size] = '\0';
                fprintf(stderr, "gpufw_init: clBuildProgram failed (%d). Build log:\n%s\n", err, log);
                free(log);
            }
        } else {
            fprintf(stderr, "gpufw_init: clBuildProgram failed (%d), no build log available\n", err);
        }
        clReleaseProgram(ctx->program);
        ctx->program = NULL;
        clReleaseCommandQueue(ctx->queue);
        clReleaseContext(ctx->context);
        ctx->queue = NULL; ctx->context = NULL;
        free(platforms);
        return err;
    }

    /* success */
    free(platforms);
    return 0;
}

/* Helper: create kernel object from ctx->program */
int gpufw_create_kernel(gpufw_ctx *ctx, const char *kernel_name, cl_kernel *out_kernel) {
    if (!ctx || !ctx->program || !kernel_name || !out_kernel) return -1;
    cl_int err;
    cl_kernel k = clCreateKernel(ctx->program, kernel_name, &err);
    if (err != CL_SUCCESS || k == NULL) {
        fprintf(stderr, "gpufw_create_kernel: clCreateKernel('%s') failed (%d)\n", kernel_name, err);
        return err;
    }
    *out_kernel = k;
    return 0;
}

/* Buffer helpers */
cl_mem gpufw_alloc_buffer(gpufw_ctx *ctx, size_t size, cl_mem_flags flags) {
    if (!ctx || !ctx->context) return NULL;
    cl_int err;
    cl_mem buf = clCreateBuffer(ctx->context, flags, size, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "gpufw_alloc_buffer: clCreateBuffer failed (%d)\n", err);
        return NULL;
    }
    return buf;
}

int gpufw_write_buffer(gpufw_ctx *ctx, cl_mem buf, const void *host_ptr, size_t size) {
    if (!ctx || !ctx->queue || !buf) return -1;
    cl_int err = clEnqueueWriteBuffer(ctx->queue, buf, CL_TRUE, 0, size, host_ptr, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "gpufw_write_buffer: clEnqueueWriteBuffer failed (%d)\n", err);
    }
    return err;
}

int gpufw_read_buffer(gpufw_ctx *ctx, cl_mem buf, void *host_ptr, size_t size) {
    if (!ctx || !ctx->queue || !buf) return -1;
    cl_int err = clEnqueueReadBuffer(ctx->queue, buf, CL_TRUE, 0, size, host_ptr, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "gpufw_read_buffer: clEnqueueReadBuffer failed (%d)\n", err);
    }
    return err;
}

/* Set kernel arg wrapper */
int gpufw_set_kernel_arg(gpufw_ctx *ctx, cl_kernel kernel, cl_uint index,
                         size_t arg_size, const void *arg_val){
    if (!kernel) return -1;
    cl_int err = clSetKernelArg(kernel, index, arg_size, arg_val);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "gpufw_set_kernel_arg: clSetKernelArg idx=%u failed (%d)\n", index, err);
    }
    return err;
}

/* Launch kernel with optional local size.
   If local_work_size == 0, pass NULL for local size letting runtime decide. */
int gpufw_launch_kernel(gpufw_ctx *ctx, cl_kernel kernel, size_t global_work_size, size_t local_work_size) {
    if (!ctx || !ctx->queue || !kernel) return -1;
    size_t gws = global_work_size;
    cl_int err;
    if (local_work_size == 0) {
        err = clEnqueueNDRangeKernel(ctx->queue, kernel, 1, NULL, &gws, NULL, 0, NULL, NULL);
    } else {
        size_t lws = local_work_size;
        err = clEnqueueNDRangeKernel(ctx->queue, kernel, 1, NULL, &gws, &lws, 0, NULL, NULL);
    }
    if (err != CL_SUCCESS) {
        fprintf(stderr, "gpufw_launch_kernel: clEnqueueNDRangeKernel failed (%d)\n", err);
        return err;
    }
    /* Wait for completion for simplicity; caller may want to use events instead */
    err = clFinish(ctx->queue);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "gpufw_launch_kernel: clFinish failed (%d)\n", err);
    }
    return err;
}

/* Cleanup all objects in ctx */
void gpufw_cleanup(gpufw_ctx *ctx) {
    if (!ctx) return;
    if (ctx->program) { clReleaseProgram(ctx->program); ctx->program = NULL; }
    if (ctx->queue)   { clReleaseCommandQueue(ctx->queue); ctx->queue = NULL; }
    if (ctx->context) { clReleaseContext(ctx->context); ctx->context = NULL; }
    /* Note: cl_device_id and cl_platform_id are not explicitly released here (platform/device are not ref-counted) */
}