// gpudrv_daemon.c
// compile: gcc -o gpudrv_daemon gpudrv_daemon.c ../libgpufw/src/gpufw.c -I../libgpufw/src -lOpenCL
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../libgpufw/src/gpufw.h"

int main() {
    int fd = open("/dev/gpudrv", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }
    if (gpufw_init(0)!=0) { fprintf(stderr,"gpufw init failed\n"); return 1; }
    while (1) {
        // read header + payload
        uint64_t len;
        ssize_t r = read(fd, (char*)&len, sizeof(len));
        if (r <= 0) { sleep(1); continue; }
        // read payload
        char *buf = malloc(len);
        ssize_t got = read(fd, buf, len); // NOTE: in our kernel, read returns header+payload in single call; this is simplified
        (void)got;
        // interpret as float array (client sends two float arrays concatenated), assume first half a, second half b
        int n = (len / sizeof(float)) / 2;
        float *a = (float*)buf;
        float *b = a + n;
        float *c = malloc(sizeof(float)*n);
        gpufw_profile_t p={0};
        gpufw_run_vecadd(a,b,c,n,&p);
        printf("Daemon processed n=%d kernel_ms=%.3f\n", n, p.kernel_ms);
        // form result: header + payload (len bytes)
        // for simplicity we send back same header then c data
        uint64_t rlen = sizeof(float)*n;
        char *out = malloc(sizeof(uint64_t) + rlen);
        memcpy(out, &rlen, sizeof(uint64_t));
        memcpy(out + sizeof(uint64_t), c, rlen);
        // write back to device (push into result queue)
        write(fd, out, sizeof(uint64_t) + rlen);
        free(out); free(c); free(buf);
    }
    gpufw_cleanup();
    close(fd);
    return 0;
}