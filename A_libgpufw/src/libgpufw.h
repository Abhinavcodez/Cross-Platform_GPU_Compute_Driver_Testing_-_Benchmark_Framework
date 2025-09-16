#ifndef LIBGPUFW_H
#define LIBGPUFW_H

#include <CL/cl.h>
#include <stddef.h>

// OpenCL context structure
typedef struct {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
} gpufw_ctx;

// Initialization from kernel file
int gpufw_init_from_file(gpufw_ctx *ctx, const char *kernel_file, int device_index);

// Buffer management
cl_mem gpufw_alloc_buffer(gpufw_ctx *ctx, size_t size, cl_mem_flags flags);
int gpufw_write_buffer(gpufw_ctx *ctx, cl_mem buf, const void *host_ptr, size_t size);
int gpufw_read_buffer(gpufw_ctx *ctx, cl_mem buf, void *host_ptr, size_t size);

// Kernel argument & launch
int gpufw_set_kernel_arg(gpufw_ctx *ctx, cl_kernel kernel, int index, size_t size, const void *value);
int gpufw_launch_kernel(gpufw_ctx *ctx, cl_kernel kernel, size_t global_work_size, size_t local_work_size);

// Cleanup
void gpufw_cleanup(gpufw_ctx *ctx);

#endif // LIBGPUFW_H