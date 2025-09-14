// gpudrv_daemon.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "../A_libgpufw/src/gpufw.h"

static int robust_write(int fd, const void *buf, size_t count) {
    size_t off = 0; const char *p = buf;
    while (off < count) {
        ssize_t w = write(fd, p + off, count - off);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        off += (size_t)w;
    }
    return 0;
}
static ssize_t robust_read(int fd, void *buf, size_t count) {
    size_t off = 0; char *p = buf;
    while (off < count) {
        ssize_t r = read(fd, p + off, count - off);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        } else if (r == 0) return (ssize_t)off;
        off += (size_t)r;
    }
    return (ssize_t)off;
}

int main(int argc, char **argv) {
    const char *dev = "/dev/gpudrv";
    int fd = open(dev, O_RDWR);
    if (fd < 0) { perror("open /dev/gpudrv"); return 1; }

    if (gpufw_init(0) != 0) { fprintf(stderr, "gpufw_init failed\n"); close(fd); return 1; }
    printf("Daemon: started, waiting for submissions...\n");

    for (;;) {
        uint64_t header;
        ssize_t r = robust_read(fd, &header, sizeof(header));
        if (r < 0) { if (errno == EINTR) continue; perror("read header"); break; }
        if (r == 0) { sleep(1); continue; }
        size_t payload_len = (size_t)header;
        if (payload_len == 0 || payload_len > (16UL*1024*1024)) { fprintf(stderr, "invalid payload_len %zu\n", payload_len); continue; }
        char *payload = malloc(payload_len);
        if (!payload) { fprintf(stderr, "alloc fail\n"); break; }
        if (robust_read(fd, payload, payload_len) != (ssize_t)payload_len) { perror("read payload"); free(payload); break; }

        // interpret payload as two float arrays: a (n), b (n)
        size_t floats = payload_len / sizeof(float);
        if (floats % 2 != 0) { fprintf(stderr, "payload floats not even\n"); free(payload); continue; }
        int n = (int)(floats / 2);
        float *a = (float *)payload;
        float *b = a + n;
        float *c = malloc(sizeof(float) * (size_t)n);
        if (!c) { fprintf(stderr, "alloc c fail\n"); free(payload); break; }

        gpufw_profile_t prof = {0};
        if (gpufw_run_vecadd(a, b, c, n, &prof) != 0) {
            fprintf(stderr, "gpufw_run_vecadd failed\n");
            free(c); free(payload); continue;
        }
        printf("Daemon: processed n=%d kernel_ms=%.3f\n", n, prof.kernel_ms);

        // prepare result: header + payload (c array)
        uint64_t rlen = (uint64_t)(sizeof(float) * (size_t)n);
        size_t total = sizeof(rlen) + (size_t)rlen;
        char *out = malloc(total);
        if (!out) { fprintf(stderr, "alloc out fail\n"); free(c); free(payload); break; }
        memcpy(out, &rlen, sizeof(rlen));
        memcpy(out + sizeof(rlen), c, (size_t)rlen);

        if (robust_write(fd, out, total) != 0) { perror("write result"); free(out); free(c); free(payload); break; }
        free(out); free(c); free(payload);
    }

    gpufw_cleanup();
    close(fd);
    return 0;
}