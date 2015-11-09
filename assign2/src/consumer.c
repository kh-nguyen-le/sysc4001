#include "common.h"

int main () {

    int shmid;
    struct buffered_stream* shm;
    char path[BUFSIZ];
    int fd;
    int total_bytes = 0;
    // create unique output
    sprintf (path, "/tmp/Consumer-%d.out", getpid());

    // try to open file
    printf ("Opening File: %s\n", path);
    fd = open (path, O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        perror ("IO Error:");
        exit (EXIT_FAILURE);
    }

    // create shared memory
    printf ("Accessing shared memory.\n");
    shmid = shmget (ftok (KEY_PATH,1), sizeof (struct buffered_stream), 0666);
    if(shmid<0)
    {
        perror ("Failed to created shared memory");
        exit (EXIT_FAILURE);
    }

    // attach this segment to virtual memory and initialise
    shm = (struct buffered_stream*) shmat (shmid, NULL, 0);
    if (shm < 0) {
        perror ("Failed to attach segement");
        exit (EXIT_FAILURE);
    }

    // open semaphores
    sem_t* mutex = sem_open (MUTEX_LOCK, 0);
    printf ("num_chunks: %d.\n", shm->num_chunks);
    sem_t* free_space = sem_open (FREE_CHUNKS, 0);
    sem_t* valid_data = sem_open (VALID_DATA, 0);
    sem_t* end_stream = sem_open (CLEANUP_LOCK, 0);

    if ((shm->mutex_toggle && mutex == SEM_FAILED) | (free_space == SEM_FAILED) | (valid_data == SEM_FAILED) |(end_stream == SEM_FAILED)) {
        perror ("Failed to create semaphore");
        exit (EXIT_FAILURE);
    }

    // starting reading from file
    printf ("Starting data stream.\n");

    while (shm->connected) {  // until no pending input
        // wait for valid semaphore then mutex lock
        sem_wait (valid_data);
        // check if stream closed while waiting
        if (!shm->connected) break;

        if (shm->mutex_toggle) sem_wait (mutex);

        unsigned int curr_length = shm->buffer[shm->read_index].bytes;
        // write chunk to file
        if (write (fd, shm->buffer[shm->read_index].text, curr_length) != curr_length) {
            fprintf (stderr, "Error: Output file corrupted.\n");
        }

        total_bytes += curr_length;

        // iterate write head and wrap around
        shm->read_index = (shm->read_index + 1) % shm->num_chunks;

        if (shm->mutex_toggle) sem_post (mutex);
            sem_post (free_space);
    }

    // Consume rest of stream without need for synchronization
    while (shm->read_index != shm->write_index) {
        unsigned int curr_length = shm->buffer[shm->read_index].bytes;
        // write chunk to file
        if (write (fd, shm->buffer[shm->read_index].text, curr_length) != curr_length) {
            fprintf (stderr, "Error: Output file corrupted.\n");
        }
        total_bytes += curr_length;
        // iterate write head and wrap around
        shm->read_index = (shm->read_index + 1) % shm->num_chunks;
    }

    // clean up
    close (fd);

    printf ("Total bytes read from stream: %d\n", total_bytes);

    sem_post (end_stream);
    sem_close (end_stream);

    sem_close (valid_data);

    sem_close (free_space);

    if (shm->mutex_toggle) {
        sem_close(mutex);
    }

    shmdt (shm);

    exit (EXIT_SUCCESS);
}