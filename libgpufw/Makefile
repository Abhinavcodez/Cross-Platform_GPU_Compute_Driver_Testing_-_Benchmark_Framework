CC = gcc
CFLAGS = -O2 -I./src
LDFLAGS = -lOpenCL
all: test_vecadd

test_vecadd: src/gpufw.c src/test_vecadd.c
	$(CC) $(CFLAGS) -o test_vecadd src/gpufw.c src/test_vecadd.c $(LDFLAGS)

clean:
	rm -f test_vecadd