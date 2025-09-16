// gpudrv_ioctl.h
#ifndef GPUDRV_IOCTL_H
#define GPUDRV_IOCTL_H

/* Simple IOCTLs for gpudrv prototype
 * Modes:
 *  0 -> CPU
 *  1 -> GPU
 *  2 -> HYBRID
 */

#include <linux/ioctl.h> /* for _IOW/_IOR on kernel side; user side will include <sys/ioctl.h> */

#define GPUDRV_IOC_MAGIC  'G'

/* Set mode (write int) */
#define GPUDRV_IOC_SET_MODE _IOW(GPUDRV_IOC_MAGIC, 1, int)
/* Get mode (read int) */
#define GPUDRV_IOC_GET_MODE _IOR(GPUDRV_IOC_MAGIC, 2, int)

#define GPUDRV_MODE_CPU    0
#define GPUDRV_MODE_GPU    1
#define GPUDRV_MODE_HYBRID 2

#endif // GPUDRV_IOCTL_H