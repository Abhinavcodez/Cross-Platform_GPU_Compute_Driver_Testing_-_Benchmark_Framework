KERNELS = kernels/vecadd.cl
LIB = libgpufw.so

all: $(LIB) test_vecadd copy_kernels

$(LIB): libgpufw.c libgpufw.h
	$(CC) -fPIC -shared -o $@ libgpufw.c -lOpenCL -ldl

test_vecadd: test_vecadd.c
	$(CC) -o $@ test_vecadd.c -L. -lgpufw -lOpenCL -ldl

copy_kernels:
	mkdir -p kernels
	cp $(KERNELS) .

install:
	sudo cp $(LIB) /usr/local/lib/
	sudo ldconfig
	sudo mkdir -p /usr/local/share/gpufw/kernels
	sudo cp $(KERNELS) /usr/local/share/gpufw/kernels/

clean:
	rm -f $(LIB) test_vecadd *.o