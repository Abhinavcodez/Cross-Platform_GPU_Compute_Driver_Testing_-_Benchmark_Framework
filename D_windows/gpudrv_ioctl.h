#ifndef GPU_DRV_IOCTL_H
#define GPU_DRV_IOCTL_H

// =====================
// Cross-Platform defines
// =====================

// IOCTL command IDs should be consistent between Linux and Windows,
// but the macros differ. We'll wrap them with #ifdef.

#define GPUDRV_IOC_MAGIC  'G'

// Command numbers (common)
#define GPUDRV_CMD_SUBMIT   1
#define GPUDRV_CMD_STATUS   2

// =====================
// Linux side
// =====================
#ifdef __linux__
#include <linux/ioctl.h>

#define GPUDRV_IOC_SUBMIT  _IOW(GPUDRV_IOC_MAGIC, GPUDRV_CMD_SUBMIT, struct gpudrv_submit)
#define GPUDRV_IOC_STATUS  _IOR(GPUDRV_IOC_MAGIC, GPUDRV_CMD_STATUS, struct gpudrv_status)

#endif // __linux__

// =====================
// Windows side
// =====================
#ifdef _WIN32
#include <winioctl.h>

// Define a custom device type (user-defined range: 0x8000–0xFFFF)
#define FILE_DEVICE_GPUDRV  0x8000

// METHOD_BUFFERED, FILE_ANY_ACCESS are standard in Windows IOCTLs
#define IOCTL_GPUDRV_SUBMIT  CTL_CODE(FILE_DEVICE_GPUDRV, GPUDRV_CMD_SUBMIT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GPUDRV_STATUS  CTL_CODE(FILE_DEVICE_GPUDRV, GPUDRV_CMD_STATUS, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif // _WIN32

// =====================
// Shared structs
// =====================
struct gpudrv_submit {
    unsigned long size;
    unsigned long id;
};

struct gpudrv_status {
    unsigned long id;
    int completed;
};

#endif // GPU_DRV_IOCTL_H