IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR) -std=c99 -D_GNU_SOURCE

LDIR = ../lib
LDFLAGS = -L$(LDIR) -levent
PKG-CONFIG = $(shell pkg-config --libs --cflags glib-2.0)

all: device controller test

device: device.c
	$(CC) -o $@ $^ $(PKG-CONFIG) $(CFLAGS) $(LDFLAGS)
	
controller: controller.c
	$(CC) -o $@ $^ $(PKG-CONFIG) $(CFLAGS) $(LDFLAGS)	

# delete test target later
test:   test_file.c
	$(CC) -o $@ $^ $(PKG-CONFIG) $(CFLAGS)

clean:
	rm -f *.o *~


