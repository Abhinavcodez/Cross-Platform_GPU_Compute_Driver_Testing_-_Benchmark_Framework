CC = gcc
CFLAGS = -Wall -O2 -I../include

all: daemon

daemon: daemon.c
	$(CC) $(CFLAGS) -o daemon daemon.c

clean:
	rm -f daemon