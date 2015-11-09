#include "common.h"

int main (int argc, char* argv[]) {

    int shmid;
    struct buffered_stream* shm;
    char path[BUFSIZ];
    int fd;
    int total_bytes = 0;
    size_t num_segments;
    int len = 0;
    char buf[BUFSIZ];
    sem_t* mutex;

    // parse command line args
    if (argc > 1) {
        sscanf(argv[1], "%s", path);
    } else {
        printf ("Must provide path to file.\n");
        exit (EXIT_FAILURE);
    }

    // try to open file
    printf ("Opening File: %s\n", path);
    fd = open (path, O_RDONLY);
    if (fd < 0) {
        perror ("IO Error:");
        exit (EXIT_FAILURE);
    }

    // create shared memory
    printf ("Accessing shared memory.\n");
    shmid = shmget (ftok (KEY_PATH,1), sizeof (struct buffered_stream), 0666);
    if(shmid<0)
    {
        perror ("Error acessing shared memory");
        exit (EXIT_FAILURE);
    }

    // attach this segment to virtual memory and initialise
    shm = (struct buffered_stream*) shmat (shmid, NULL, 0);
    if (shm < 0) {
        perror ("Failed to attach segment");
        exit (EXIT_FAILURE);
    }

    // open semaphores
    if (shm->mutex_toggle) mutex = sem_open (MUTEX_LOCK, O_CREAT, 0644, 1);
    sem_t* free_space = sem_open (FREE_CHUNKS, O_CREAT, 0644, (size_t)(shm->num_chunks));
    sem_t* valid_data = sem_open (VALID_DATA, O_CREAT, 0644, 0);
    sem_t* end_stream = sem_open (CLEANUP_LOCK, 0);

    if ((shm->mutex_toggle && mutex == SEM_FAILED) | (free_space == SEM_FAILED) | (valid_data == SEM_FAILED) |(end_stream == SEM_FAILED)) {
        perror ("Failed to create semaphore");
        exit (EXIT_FAILURE);
    }

    // enumerate data segments
    unsigned int chunk_size = shm->chunk_size;
    num_segments = BUFSIZ / chunk_size;

    do {
        // starting reading from file
        printf ("Getting new input.\n");
        len = (int)read (fd, buf, BUFSIZ);
        if (len == -1) {
            perror ("IO Error: ");
            close (fd);
            exit (EXIT_FAILURE);
        } else if (len == 0) {
            printf ("Unexected empty input. Ending stream.\n");
            sem_post (valid_data);
            break;
        }

        // write segments to stream sequentially
        int count = 0;
        int current = 0;
        printf ("Writing to buffer.\n");
        while (count < num_segments && current < len) {
            // wait for empty semaphore then mutex lock
            sem_wait (free_space);
            if (shm->mutex_toggle) sem_wait (mutex);

            // write current segment to stream
            for (int i = 0; i < chunk_size; i++) {
                shm->buffer[shm->write_index].text[i] = buf[current++];
                if (current == len) break;
            }

            // record bytes written to stream chunk
            shm->buffer[shm->write_index].bytes = (int) strlen (shm->buffer[shm->write_index].text);
            total_bytes += shm->buffer[shm->write_index].bytes;

            // iterate write head and wrap around
            shm->write_index = (shm->write_index + 1) % shm->num_chunks;

            if (shm->mutex_toggle) sem_post (mutex);
            sem_post (valid_data);
            count++;
        }
    } while (len == BUFSIZ);  // until EOF

    shm->connected = 0;

    // clean up
    close (fd);

    printf ("Total bytes written to stream: %d\n", total_bytes);

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