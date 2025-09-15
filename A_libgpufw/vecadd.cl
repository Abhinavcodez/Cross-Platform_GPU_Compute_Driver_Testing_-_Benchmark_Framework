// vecadd.cl
// Simple vector addition: c[i] = a[i] + b[i]
__kernel void vecadd(__global const float *a,
                     __global const float *b,
                     __global float *c,
                     const int n)
{
    int gid = get_global_id(0);
    if (gid < n) c[gid] = a[gid] + b[gid];
}