CC = gcc
CFLAGS = -O2 -Wall -I../../A_libgpufw/src
LDFLAGS = -lOpenCL
LIBGPUDIR = ../../A_libgpufw/src

all: gpudrv_client gpudrv_daemon

gpudrv_client: gpudrv_client.c
	$(CC) $(CFLAGS) -o gpudrv_client gpudrv_client.c

# daemon links static gpufw for simplicity (or you can compile shared lib)
gpudrv_daemon: gpudrv_daemon.c $(LIBGPUDIR)/gpufw.c $(LIBGPUDIR)/gpufw.h
	$(CC) $(CFLAGS) -o gpudrv_daemon gpudrv_daemon.c $(LIBGPUDIR)/gpufw.c $(LDFLAGS)

clean:
	rm -f gpudrv_client gpudrv_daemon