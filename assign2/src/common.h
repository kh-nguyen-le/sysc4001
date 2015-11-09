#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define MAX_CHUNKS 150

#define KEY_PATH  "."
#define MUTEX_LOCK "/MUTEX_LOCK"
#define FREE_CHUNKS  "/FREE_CHUNKS"
#define VALID_DATA "/VALID_DATA"
#define CLEANUP_LOCK "/CLEANUP_LOCK"

struct chunk_t {
    char text[BUFSIZ];
    unsigned int bytes;
};

struct buffered_stream {
    struct chunk_t buffer[MAX_CHUNKS];
    unsigned int num_chunks;
    unsigned int chunk_size;
    int mutex_toggle;
    unsigned int read_index;
    unsigned int write_index;
    int connected;
};