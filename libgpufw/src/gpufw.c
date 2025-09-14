// gpufw.c  (compile with -lOpenCL)
#define CL_TARGET_OPENCL_VERSION 120
#include "gpufw.h"
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static cl_platform_id platform = NULL;
static cl_device_id device = NULL;
static cl_context ctx = NULL;
static cl_command_queue cq = NULL;
static cl_program program = NULL;
static cl_kernel vecadd_kernel = NULL;

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file %s\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END); 
    long sz = ftell(f); 
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf) {
        fclose(f);
        fprintf(stderr, "Error: Memory allocation failed for file %s\n", path);
        return NULL;
    }
    size_t bytes_read = fread(buf, 1, sz, f);
    buf[bytes_read] = 0;
    fclose(f);
    return buf;
}

void print_opencl_error(cl_int err) {
    switch(err) {
        case CL_SUCCESS: fprintf(stderr, "CL_SUCCESS\n"); break;
        case CL_DEVICE_NOT_FOUND: fprintf(stderr, "CL_DEVICE_NOT_FOUND\n"); break;
        case CL_DEVICE_NOT_AVAILABLE: fprintf(stderr, "CL_DEVICE_NOT_AVAILABLE\n"); break;
        case CL_COMPILER_NOT_AVAILABLE: fprintf(stderr, "CL_COMPILER_NOT_AVAILABLE\n"); break;
        case CL_MEM_OBJECT_ALLOCATION_FAILURE: fprintf(stderr, "CL_MEM_OBJECT_ALLOCATION_FAILURE\n"); break;
        case CL_OUT_OF_RESOURCES: fprintf(stderr, "CL_OUT_OF_RESOURCES\n"); break;
        case CL_OUT_OF_HOST_MEMORY: fprintf(stderr, "CL_OUT_OF_HOST_MEMORY\n"); break;
        case CL_PROFILING_INFO_NOT_AVAILABLE: fprintf(stderr, "CL_PROFILING_INFO_NOT_AVAILABLE\n"); break;
        case CL_MEM_COPY_OVERLAP: fprintf(stderr, "CL_MEM_COPY_OVERLAP\n"); break;
        case CL_IMAGE_FORMAT_MISMATCH: fprintf(stderr, "CL_IMAGE_FORMAT_MISMATCH\n"); break;
        case CL_IMAGE_FORMAT_NOT_SUPPORTED: fprintf(stderr, "CL_IMAGE_FORMAT_NOT_SUPPORTED\n"); break;
        case CL_BUILD_PROGRAM_FAILURE: fprintf(stderr, "CL_BUILD_PROGRAM_FAILURE\n"); break;
        case CL_MAP_FAILURE: fprintf(stderr, "CL_MAP_FAILURE\n"); break;
        case CL_INVALID_VALUE: fprintf(stderr, "CL_INVALID_VALUE\n"); break;
        case CL_INVALID_DEVICE_TYPE: fprintf(stderr, "CL_INVALID_DEVICE_TYPE\n"); break;
        case CL_INVALID_PLATFORM: fprintf(stderr, "CL_INVALID_PLATFORM\n"); break;
        case CL_INVALID_DEVICE: fprintf(stderr, "CL_INVALID_DEVICE\n"); break;
        case CL_INVALID_CONTEXT: fprintf(stderr, "CL_INVALID_CONTEXT\n"); break;
        case CL_INVALID_QUEUE_PROPERTIES: fprintf(stderr, "CL_INVALID_QUEUE_PROPERTIES\n"); break;
        case CL_INVALID_COMMAND_QUEUE: fprintf(stderr, "CL_INVALID_COMMAND_QUEUE\n"); break;
        case CL_INVALID_HOST_PTR: fprintf(stderr, "CL_INVALID_HOST_PTR\n"); break;
        case CL_INVALID_MEM_OBJECT: fprintf(stderr, "CL_INVALID_MEM_OBJECT\n"); break;
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: fprintf(stderr, "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR\n"); break;
        case CL_INVALID_IMAGE_SIZE: fprintf(stderr, "CL_INVALID_IMAGE_SIZE\n"); break;
        case CL_INVALID_SAMPLER: fprintf(stderr, "CL_INVALID_SAMPLER\n"); break;
        case CL_INVALID_BINARY: fprintf(stderr, "CL_INVALID_BINARY\n"); break;
        case CL_INVALID_BUILD_OPTIONS: fprintf(stderr, "CL_INVALID_BUILD_OPTIONS\n"); break;
        case CL_INVALID_PROGRAM: fprintf(stderr, "CL_INVALID_PROGRAM\n"); break;
        case CL_INVALID_PROGRAM_EXECUTABLE: fprintf(stderr, "CL_INVALID_PROGRAM_EXECUTABLE\n"); break;
        case CL_INVALID_KERNEL_NAME: fprintf(stderr, "CL_INVALID_KERNEL_NAME\n"); break;
        case CL_INVALID_KERNEL_DEFINITION: fprintf(stderr, "CL_INVALID_KERNEL_DEFINITION\n"); break;
        case CL_INVALID_KERNEL: fprintf(stderr, "CL_INVALID_KERNEL\n"); break;
        case CL_INVALID_ARG_INDEX: fprintf(stderr, "CL_INVALID_ARG_INDEX\n"); break;
        case CL_INVALID_ARG_VALUE: fprintf(stderr, "CL_INVALID_ARG_VALUE\n"); break;
        case CL_INVALID_ARG_SIZE: fprintf(stderr, "CL_INVALID_ARG_SIZE\n"); break;
        case CL_INVALID_KERNEL_ARGS: fprintf(stderr, "CL_INVALID_KERNEL_ARGS\n"); break;
        case CL_INVALID_WORK_DIMENSION: fprintf(stderr, "CL_INVALID_WORK_DIMENSION\n"); break;
        case CL_INVALID_WORK_GROUP_SIZE: fprintf(stderr, "CL_INVALID_WORK_GROUP_SIZE\n"); break;
        case CL_INVALID_WORK_ITEM_SIZE: fprintf(stderr, "CL_INVALID_WORK_ITEM_SIZE\n"); break;
        case CL_INVALID_GLOBAL_OFFSET: fprintf(stderr, "CL_INVALID_GLOBAL_OFFSET\n"); break;
        case CL_INVALID_EVENT_WAIT_LIST: fprintf(stderr, "CL_INVALID_EVENT_WAIT_LIST\n"); break;
        case CL_INVALID_EVENT: fprintf(stderr, "CL_INVALID_EVENT\n"); break;
        case CL_INVALID_OPERATION: fprintf(stderr, "CL_INVALID_OPERATION\n"); break;
        case CL_INVALID_GL_OBJECT: fprintf(stderr, "CL_INVALID_GL_OBJECT\n"); break;
        case CL_INVALID_BUFFER_SIZE: fprintf(stderr, "CL_INVALID_BUFFER_SIZE\n"); break;
        case CL_INVALID_MIP_LEVEL: fprintf(stderr, "CL_INVALID_MIP_LEVEL\n"); break;
        case CL_INVALID_GLOBAL_WORK_SIZE: fprintf(stderr, "CL_INVALID_GLOBAL_WORK_SIZE\n"); break;
        default: fprintf(stderr, "Unknown OpenCL error: %d\n", err); break;
    }
}

int gpufw_init(int device_index) {
    cl_int err;
    
    // Get platforms
    cl_uint nplat;
    err = clGetPlatformIDs(1, &platform, &nplat);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clGetPlatformIDs failed: ");
        print_opencl_error(err);
        return -1;
    }
    fprintf(stderr, "Found %d platform(s)\n", nplat);
    
    // Get devices
    cl_uint ndev;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &ndev);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clGetDeviceIDs (count) failed: ");
        print_opencl_error(err);
        return -1;
    }
    fprintf(stderr, "Found %d device(s)\n", ndev);
    
    if (ndev == 0) {
        fprintf(stderr, "Error: No OpenCL devices found\n");
        return -1;
    }
    
    cl_device_id *devices = malloc(ndev * sizeof(cl_device_id));
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, ndev, devices, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clGetDeviceIDs failed: ");
        print_opencl_error(err);
        free(devices);
        return -1;
    }
    
    // Select device
    if (device_index >= 0 && device_index < (int)ndev) {
        device = devices[device_index];
        fprintf(stderr, "Using device index: %d\n", device_index);
    } else {
        device = devices[0]; // Use first device by default
        fprintf(stderr, "Using default device (index 0)\n");
    }
    
    // Print device info
    char device_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    fprintf(stderr, "Using device: %s\n", device_name);
    
    free(devices);
    
    // Create context
    ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!ctx || err != CL_SUCCESS) {
        fprintf(stderr, "clCreateContext failed: ");
        print_opencl_error(err);
        return -1;
    }
    
    // Create command queue
    cq = clCreateCommandQueue(ctx, device, CL_QUEUE_PROFILING_ENABLE, &err);
    if (!cq || err != CL_SUCCESS) {
        fprintf(stderr, "clCreateCommandQueue failed: ");
        print_opencl_error(err);
        return -1;
    }
    
    // Read kernel source - try multiple paths
    char *src = NULL;
    const char *paths[] = {
        "../kernels/vecadd.cl",
        "kernels/vecadd.cl",
        "./kernels/vecadd.cl",
        "vecadd.cl",
        NULL
    };
    
    for (int i = 0; paths[i] != NULL; i++) {
        src = read_file(paths[i]);
        if (src) {
            fprintf(stderr, "Found kernel file: %s\n", paths[i]);
            break;
        }
    }
    
    if(!src) {
        fprintf(stderr, "Error: Failed to find kernel file. Tried:\n");
        for (int i = 0; paths[i] != NULL; i++) {
            fprintf(stderr, "  %s\n", paths[i]);
        }
        return -1;
    }
    
    // Create program
    program = clCreateProgramWithSource(ctx, 1, (const char**)&src, NULL, &err);
    free(src);
    if (!program || err != CL_SUCCESS) {
        fprintf(stderr, "clCreateProgramWithSource failed: ");
        print_opencl_error(err);
        return -1;
    }
    
    // Build program
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clBuildProgram failed: ");
        print_opencl_error(err);
        
        // Get build log
        size_t len; 
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
        if (len > 0) {
            char *log = malloc(len + 1); 
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, len, log, NULL); 
            log[len] = 0;
            fprintf(stderr, "Build log:\n%s\n", log); 
            free(log);
        } else {
            fprintf(stderr, "No build log available\n");
        }
        return -1;
    }
    
    // Create kernel
    vecadd_kernel = clCreateKernel(program, "vecadd", &err);
    if (!vecadd_kernel || err != CL_SUCCESS) {
        fprintf(stderr, "clCreateKernel failed: ");
        print_opencl_error(err);
        return -1;
    }
    
    fprintf(stderr, "GPU framework initialized successfully!\n");
    return 0;
}

int gpufw_run_vecadd(const float *a, const float *b, float *c, int n, gpufw_profile_t *prof) {
    if (n <= 0) {
        fprintf(stderr, "Error: Invalid vector size %d\n", n);
        return -1;
    }
    
    if (!vecadd_kernel) {
        fprintf(stderr, "Error: Kernel not initialized\n");
        return -1;
    }
    
    cl_int err;
    size_t bytes = sizeof(float) * n;
    
    // Create buffers
    cl_mem buf_a = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bytes, (void*)a, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clCreateBuffer (a) failed: ");
        print_opencl_error(err);
        return -1;
    }
    
    cl_mem buf_b = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bytes, (void*)b, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clCreateBuffer (b) failed: ");
        print_opencl_error(err);
        clReleaseMemObject(buf_a);
        return -1;
    }
    
    cl_mem buf_c = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, bytes, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clCreateBuffer (c) failed: ");
        print_opencl_error(err);
        clReleaseMemObject(buf_a);
        clReleaseMemObject(buf_b);
        return -1;
    }

    // Set kernel arguments
    if (clSetKernelArg(vecadd_kernel, 0, sizeof(cl_mem), &buf_a) != CL_SUCCESS) {
        fprintf(stderr, "clSetKernelArg (a) failed\n");
        clReleaseMemObject(buf_a); clReleaseMemObject(buf_b); clReleaseMemObject(buf_c);
        return -1;
    }
    if (clSetKernelArg(vecadd_kernel, 1, sizeof(cl_mem), &buf_b) != CL_SUCCESS) {
        fprintf(stderr, "clSetKernelArg (b) failed\n");
        clReleaseMemObject(buf_a); clReleaseMemObject(buf_b); clReleaseMemObject(buf_c);
        return -1;
    }
    if (clSetKernelArg(vecadd_kernel, 2, sizeof(cl_mem), &buf_c) != CL_SUCCESS) {
        fprintf(stderr, "clSetKernelArg (c) failed\n");
        clReleaseMemObject(buf_a); clReleaseMemObject(buf_b); clReleaseMemObject(buf_c);
        return -1;
    }
    if (clSetKernelArg(vecadd_kernel, 3, sizeof(int), &n) != CL_SUCCESS) {
        fprintf(stderr, "clSetKernelArg (n) failed\n");
        clReleaseMemObject(buf_a); clReleaseMemObject(buf_b); clReleaseMemObject(buf_c);
        return -1;
    }

    // Calculate work sizes
    size_t gsize = ((size_t)n + 63) & ~((size_t)63);
    size_t local_size = 64;
    
    if (local_size > gsize) {
        local_size = gsize;
    }

    cl_event ev;
    // Enqueue kernel with profiling
    err = clEnqueueNDRangeKernel(cq, vecadd_kernel, 1, NULL, &gsize, &local_size, 0, NULL, &ev);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clEnqueueNDRangeKernel failed: ");
        print_opencl_error(err);
        clReleaseMemObject(buf_a); clReleaseMemObject(buf_b); clReleaseMemObject(buf_c);
        return -1;
    }
    
    // Wait for completion
    err = clWaitForEvents(1, &ev);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clWaitForEvents failed: ");
        print_opencl_error(err);
        clReleaseMemObject(buf_a); clReleaseMemObject(buf_b); clReleaseMemObject(buf_c);
        clReleaseEvent(ev);
        return -1;
    }

    // Profiling
    if (prof) {
        cl_ulong start, end;
        clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
        clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL);
        prof->kernel_ms = (double)(end - start) * 1e-6;
        fprintf(stderr, "Kernel execution time: %.3f ms\n", prof->kernel_ms);
    }

    // Read back results
    err = clEnqueueReadBuffer(cq, buf_c, CL_TRUE, 0, bytes, c, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clEnqueueReadBuffer failed: ");
        print_opencl_error(err);
        clReleaseMemObject(buf_a); clReleaseMemObject(buf_b); clReleaseMemObject(buf_c);
        clReleaseEvent(ev);
        return -1;
    }

    // Cleanup
    clReleaseEvent(ev);
    clReleaseMemObject(buf_a);
    clReleaseMemObject(buf_b);
    clReleaseMemObject(buf_c);
    
    return 0;
}

void gpufw_cleanup(void) {
    if (vecadd_kernel) {
        clReleaseKernel(vecadd_kernel);
        vecadd_kernel = NULL;
    }
    if (program) {
        clReleaseProgram(program);
        program = NULL;
    }
    if (cq) {
        clReleaseCommandQueue(cq);
        cq = NULL;
    }
    if (ctx) {
        clReleaseContext(ctx);
        ctx = NULL;
    }
    fprintf(stderr, "GPU framework cleaned up\n");
}