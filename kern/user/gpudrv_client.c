// gpudrv_client.c
// compile: gcc -o gpudrv_client gpudrv_client.c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int n = 1024*1024;
    int fd = open("/dev/gpudrv", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }
    size_t bytes = sizeof(float) * n * 2;
    char *buf = malloc(sizeof(uint64_t) + bytes);
    uint64_t len = bytes;
    memcpy(buf, &len, sizeof(uint64_t));
    float *a = (float*)(buf + sizeof(uint64_t));
    float *b = a + n;
    for (int i=0;i<n;i++){ a[i]=i; b[i]=2*i; }
    // submit
    if (write(fd, buf, sizeof(uint64_t) + bytes) < 0) { perror("write"); return 1; }
    // wait for result
    uint64_t rlen;
    if (read(fd, (char*)&rlen, sizeof(rlen)) <= 0) { perror("read header"); return 1; }
    char *out = malloc(rlen);
    if (read(fd, out, rlen) <= 0) { perror("read payload"); return 1; }
    float *c = (float*)out;
    printf("c[0]=%f c[1]=%f c[last]=%f\n", c[0], c[1], c[n-1]);
    free(buf); free(out); close(fd);
    return 0;
}