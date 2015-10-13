IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR) -std=c99 -D_GNU_SOURCE

LDIR = ../lib
LDFLAGS = -L$(LDIR) -levent
PKG-CONFIG = $(shell pkg-config --libs --cflags glib-2.0)

all: device controller cloud

device: device.c
	$(CC) -o $@ $^ $(PKG-CONFIG) $(CFLAGS) $(LDFLAGS)
	
controller: controller.c
	$(CC) -o $@ $^ $(PKG-CONFIG) $(CFLAGS)	
	
cloud:   cloud.c
	$(CC) -o $@ $^ $(PKG-CONFIG) $(CFLAGS)

clean:
	rm -f *.o *~


