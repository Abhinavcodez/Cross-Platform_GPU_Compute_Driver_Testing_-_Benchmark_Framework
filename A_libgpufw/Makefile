KERNELS = kernels/vecadd.cl
LIB     = libgpufw.so

CC      = gcc
CFLAGS  = -Wall -fPIC -I./src -DCL_TARGET_OPENCL_VERSION=200
LDFLAGS = -lOpenCL -ldl

all: $(LIB) test_vecadd copy_kernels

$(LIB): src/libgpufw.c src/libgpufw.h
	$(CC) $(CFLAGS) -shared -o $(LIB) src/libgpufw.c $(LDFLAGS)

test_vecadd: test_vecadd.c src/libgpufw.h $(LIB)
	$(CC) $(CFLAGS) -o test_vecadd test_vecadd.c -L. -lgpufw $(LDFLAGS)

# Copy kernels
copy_kernels:
	@echo "kernels already in place, nothing to copy."

install: $(LIB)
	sudo cp $(LIB) /usr/local/lib/
	sudo ldconfig
	sudo mkdir -p /usr/local/share/gpufw/kernels
	sudo cp $(KERNELS) /usr/local/share/gpufw/kernels/

clean:
	rm -f $(LIB) test_vecadd *.o