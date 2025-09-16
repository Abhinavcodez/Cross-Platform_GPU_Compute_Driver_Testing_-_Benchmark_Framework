#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../include/gpudrv_ioctl.h"

int main() {
    int fd = open("/dev/gpudrv", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("Requesting CPU mode...\n");
    ioctl(fd, GPUDRV_IOC_RUN_CPU);

    printf("Requesting GPU mode...\n");
    ioctl(fd, GPUDRV_IOC_RUN_GPU);

    printf("Requesting Hybrid mode...\n");
    ioctl(fd, GPUDRV_IOC_RUN_HYBRID);

    close(fd);
    return 0;
}