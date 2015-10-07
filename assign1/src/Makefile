IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR) -std=c99 -D_GNU_SOURCE

LDIR =../lib
LDFLAGS = -L$(LDIR) -levent_core

all: device test

device: device.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

# delete test target later
test:   test_file.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o *~


