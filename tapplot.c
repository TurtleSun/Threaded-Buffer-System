// tapplot

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <buffer.h>

#define PLOT_KEY 5678
#define SHMSIZE 100000

int main(){
    // open shared memory that we initialized in tapper between reconstruct and tapplot
    int shm_fd_rectap = shmget(PLOT_KEY, SHMSIZE, 0666);
    void * shm_rectap_addr = shmat(shm_fd_rectap, NULL, 0);

    if (shm_fd_rectap == NULL) {
        // something went horribly wrong
        perror("shmatt error");
        exit(1);
    }

    // read from shared memory
    // ... TODO: finish this

    // close shared memory
    // detach it
    shmdt(shm_rectap_addr);

    // exit
    return 0;
}