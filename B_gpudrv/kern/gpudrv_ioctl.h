// gpudrv_ioctl.h
#ifndef GPUDRV_IOCTL_H
#define GPUDRV_IOCTL_H

#include <linux/ioctl.h>

#define GPUDRV_IOC_MAGIC  'G'

// Submit: user writes header+payload using write(), or calls ioctl with metadata.
// For this simple prototype we only define a COMPLETE IOCTL for potential extension.
#define GPUDRV_IOC_COMPLETE _IOW(GPUDRV_IOC_MAGIC, 1, unsigned long)

#endif // GPUDRV_IOCTL_H