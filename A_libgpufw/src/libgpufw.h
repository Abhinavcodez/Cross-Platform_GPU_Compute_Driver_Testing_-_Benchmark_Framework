#ifndef LIBGPUFW_H
#define LIBGPUFW_H

#include <CL/cl.h>

// Context structure to hold OpenCL objects
typedef struct {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
} gpufw_ctx;

// ==========================
// Public API
// ==========================

// Initialize with kernel source from a file
int gpufw_init_from_file(gpufw_ctx *ctx, const char *kernel_path,
                         const char *kernel_name, int dev_index);

// Initialize with kernel source from a string
int gpufw_init_from_string(gpufw_ctx *ctx, const char *kernel_src,
                           const char *kernel_name, int dev_index);

// Allocate/free buffers
cl_mem gpufw_alloc_buffer(gpufw_ctx *ctx, size_t size, cl_mem_flags flags);
void gpufw_free_buffer(cl_mem buf);

// Data transfer
int gpufw_write_buffer(gpufw_ctx *ctx, cl_mem buf, const void *host_ptr, size_t size);
int gpufw_read_buffer(gpufw_ctx *ctx, cl_mem buf, void *host_ptr, size_t size);

// Set kernel argument
int gpufw_set_kernel_arg(gpufw_ctx *ctx, int index, size_t size, const void *value);

// Launch kernel
int gpufw_launch_kernel(gpufw_ctx *ctx, size_t global_work_size, size_t local_work_size);

// Cleanup
void gpufw_cleanup(gpufw_ctx *ctx);

#endif // LIBGPUFW_H