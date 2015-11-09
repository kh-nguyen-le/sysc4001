#include "common.h"

int main (int argc, char* argv[]) {

    unsigned int num_chunks = 100;
    unsigned int chunk_size = 128;
    int mutex_toggle = 1;
    int shmid;
    struct buffered_stream* shm;

    // parse command line args
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            int digit;
            switch (argv[i][1]) {
                case 'n': // number of chunks to use
                    digit = atoi (argv[++i]);
                        num_chunks = 0;
                        num_chunks +=digit;
                    break;
                case 's': // size of each chunk
                    if (sscanf (argv[++i], "%i", &digit)) {
                        chunk_size = 0;
                        chunk_size +=digit;
                    }
                    break;
                case 't': // turn off mutex lock
                    mutex_toggle = 0;
                    break;
                default:
                    break;
            }

        } //else do nothing
    }

    // check user-defined values
    if (num_chunks > MAX_CHUNKS || chunk_size > BUFSIZ) {
        printf ("Invalid parameters. \n");
        exit (EXIT_FAILURE);
    }



    // create shared memory
    printf ("Creating shared memory.\n");
    shmid = shmget (ftok (KEY_PATH,1), sizeof (struct buffered_stream), IPC_CREAT | 0666);
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

    shm->chunk_size = chunk_size;
    shm->num_chunks = num_chunks;
    shm->mutex_toggle = mutex_toggle;
    shm->connected = 1;
    shm->read_index = 0;
    shm->write_index = 0;

    // create semaphores and wait for connection to finish before cleaning
    sem_t* end_stream = sem_open (CLEANUP_LOCK, O_CREAT, 0644, 0);

    if (end_stream == SEM_FAILED) {
        perror ("Failed to create semaphore");
        exit (EXIT_FAILURE);
    }

    // wait for both ends to signal termination
    printf ("Waiting until transfer completes.\n");

    sem_wait (end_stream);
    printf ("Waiting until second endpoint disconnects.\n");
    sem_wait (end_stream);

    // clean up
    sem_close (end_stream);
    sem_unlink (CLEANUP_LOCK);
    sem_unlink (VALID_DATA);
    sem_unlink (FREE_CHUNKS);

    if (mutex_toggle) {
        sem_unlink(MUTEX_LOCK);
    }

    shmctl (shmid, IPC_RMID, 0);

    exit(EXIT_SUCCESS);
}