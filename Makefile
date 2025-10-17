CC=g++
CFLAGS=-g -pthread
LDFLAGS=-lpthread

all: build

compile_src:
	$(CC) -c $(CFLAGS) src/*.c

shalink: compile_src
	rm -f libshalink.a
	ar crs libshalink.a *.o
	rm -f *.o

build: test/main.c shalink
	$(CC) -o shalink.bin test/main.c libshalink.a $(CFLAGS) $(LDFLAGS)

run: build
	./shalink.bin

clean:
	rm -rf *.o .a shalink.bin
