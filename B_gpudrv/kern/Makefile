CC = gcc
CFLAGS = -Wall -O2

all: gpudrv_daemon gpudrv_client

gpudrv_daemon: daemon.c gpudrv_ioctl.h
	$(CC) $(CFLAGS) -o $@ daemon.c

gpudrv_client: client.c gpudrv_ioctl.h
	$(CC) $(CFLAGS) -o $@ client.c

clean:
	rm -f gpudrv_daemon gpudrv_client