#ifndef GPUDRV_IOCTL_H
#define GPUDRV_IOCTL_H

#include <linux/ioctl.h>

#define GPUDRV_IOC_MAGIC   'G'

#define GPUDRV_IOC_RUN_CPU     _IO(GPUDRV_IOC_MAGIC, 1)
#define GPUDRV_IOC_RUN_GPU     _IO(GPUDRV_IOC_MAGIC, 2)
#define GPUDRV_IOC_RUN_HYBRID  _IO(GPUDRV_IOC_MAGIC, 3)

#endif