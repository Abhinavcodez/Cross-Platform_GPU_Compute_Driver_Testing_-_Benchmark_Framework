CC = gcc
CFLAGS = -Wall -O2 -fPIC -DCL_TARGET_OPENCL_VERSION=200
LDFLAGS = -lOpenCL

# source files
LIBSRC = gpufw_opencl.c
LIBOBJ = $(LIBSRC:.c=.o)
LIBNAME = libgpufw.so

TESTSRC = test_vecadd.c
TESTOBJ = $(TESTSRC:.c=.o)
TESTBIN = test_vecadd

all: $(LIBNAME) $(TESTBIN)

# build shared library
$(LIBNAME): $(LIBOBJ)
	$(CC) -shared -o $@ $(LIBOBJ) $(LDFLAGS)

# build test binary
$(TESTBIN): $(TESTOBJ) $(LIBNAME)
	$(CC) -o $@ $(TESTOBJ) -L. -lgpufw $(LDFLAGS)

# generic compile rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIBOBJ) $(TESTOBJ) $(LIBNAME) $(TESTBIN)
