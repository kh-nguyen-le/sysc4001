CC=gcc
CFLAGS= -std=c99 -D_GNU_SOURCE

LDFLAGS = -lpthread

all: host producer consumer

host:	src/host.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)
	
producer:	src/producer.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)
	
consumer:	src/consumer.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o *~


