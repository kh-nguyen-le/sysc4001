IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR) -std=c99 -D_GNU_SOURCE $(shell pkg-config --cflags glib-2.0)

LDIR = ../lib
LDFLAGS = -L$(LDIR) -levent_core $(shell pkg-config --libs glib-2.0)


all: device controller test

device: device.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)
	
controller: controller.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)	

# delete test target later
test:   test_file.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o *~


