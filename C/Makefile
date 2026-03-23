CC=g++
CFLAGS=-g -pthread
LDFLAGS=-lpthread -lm

all: build

compile:
	$(CC) -c $(CFLAGS) src/*.c

shalink: compile
	rm -f libshalink.a
	ar crs libshalink.a *.o
	rm -f *.o

build: main.c shalink
	$(CC) main.c libshalink.a $(CFLAGS) $(LDFLAGS) -o shalink

run: build
	./shalink

clean:
	rm -rf *.o .a shalink
