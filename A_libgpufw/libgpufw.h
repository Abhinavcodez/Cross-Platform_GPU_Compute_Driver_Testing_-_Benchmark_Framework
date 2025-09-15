// gpufw.h
#ifndef GPUFW_H
#define GPUFW_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double kernel_ms;
    double transfer_ms; // reserved (not computed in this simple impl)
} gpufw_profile_t;

int gpufw_init(int device_index);
int gpufw_run_vecadd(const float *a, const float *b, float *c, int n, gpufw_profile_t *prof);
void gpufw_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // GPUFW_H