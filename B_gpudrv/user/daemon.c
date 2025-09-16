// daemon.c (user-space)
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "../include/gpudrv_ioctl.h" // copy/symlink from kern/

static void do_set_mode(int fd, int mode, const char *name)
{
    printf("Requesting %s mode...\n", name);
    if (ioctl(fd, GPUDRV_IOC_SET_MODE, &mode) == -1) {
        fprintf(stderr, "ioctl SET_MODE(%s) failed: %s\n", name, strerror(errno));
    } else {
        int cur = -1;
        if (ioctl(fd, GPUDRV_IOC_GET_MODE, &cur) == -1) {
            fprintf(stderr, "ioctl GET_MODE failed: %s\n", strerror(errno));
        } else {
            printf("Kernel reports current mode = %d\n", cur);
        }
    }
}

int main(int argc, char **argv)
{
    const char *dev = "/dev/gpudrv";
    int fd = open(dev, O_RDWR);
    if (fd < 0) {
        perror("open /dev/gpudrv");
        return 1;
    }

    do_set_mode(fd, GPUDRV_MODE_CPU, "CPU");
    sleep(1);
    do_set_mode(fd, GPUDRV_MODE_GPU, "GPU");
    sleep(1);
    do_set_mode(fd, GPUDRV_MODE_HYBRID, "HYBRID");

    close(fd);
    return 0;
}