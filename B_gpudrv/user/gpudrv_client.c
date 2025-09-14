// gpudrv_client.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int robust_write(int fd, const void *buf, size_t count) {
    size_t off = 0;
    const char *p = buf;
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
int robust_read(int fd, void *buf, size_t count) {
    size_t off = 0;
    char *p = buf;
    while (off < count) {
        ssize_t r = read(fd, p + off, count - off);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        } else if (r == 0) return (int)off;
        off += (size_t)r;
    }
    return (int)off;
}

int main(int argc, char **argv) {
    const char *dev = "/dev/gpudrv";
    int fd = open(dev, O_RDWR);
    if (fd < 0) { perror("open /dev/gpudrv"); return 1; }

    int n = 1024*1024;
    if (argc > 1) n = atoi(argv[1]);
    size_t payload_bytes = sizeof(float) * (size_t)n * 2; // a and b concatenated
    uint64_t header = (uint64_t)payload_bytes;
    size_t total = sizeof(header) + payload_bytes;
    char *buf = malloc(total);
    if (!buf) { fprintf(stderr, "alloc fail\n"); close(fd); return 1; }
    memcpy(buf, &header, sizeof(header));
    float *a = (float *)(buf + sizeof(header));
    float *b = a + n;
    for (int i = 0; i < n; ++i) { a[i] = (float)i; b[i] = (float)(2*i); }

    if (robust_write(fd, buf, total) != 0) { perror("write submit"); free(buf); close(fd); return 1; }
    free(buf);

    // block for response: response is header(uint64) + payload (n floats)
    uint64_t rlen;
    if (robust_read(fd, &rlen, sizeof(rlen)) != sizeof(rlen)) { perror("read header"); close(fd); return 1; }
    if (rlen != sizeof(float) * (size_t)n) {
        fprintf(stderr, "unexpected rlen %llu expected %zu\n", (unsigned long long)rlen, sizeof(float) * (size_t)n);
        close(fd); return 1;
    }
    float *out = malloc((size_t)rlen);
    if (!out) { fprintf(stderr, "alloc fail\n"); close(fd); return 1; }
    if (robust_read(fd, out, (size_t)rlen) != (int)rlen) { perror("read payload"); free(out); close(fd); return 1; }

    printf("c[0]=%g c[1]=%g c[last]=%g\n", out[0], out[1], out[n-1]);
    free(out);
    close(fd);
    return 0;
}