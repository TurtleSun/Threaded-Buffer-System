// P2's task is to execute a program called reconstruct, which reconstructs the missing data not sent from P1. 
// Using the above example, since P2 receives "temperature=60 F",  followed by "time=10 s", then "time=20 s" it knows that 
// "temperature=60 F" is the last such "name=value" pair at "time=20 s". 
// P2's reconstruction process will create a table of samples for all "name=value" pairs seen so far.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <buffer.h>

// definitions

#define OBSERVE_KEY 1234
#define PLOT_KEY 5678
#define SHMSIZE 100000

int main(){
    // open shared memory that we initialized in tapper between observe and reconstruct
    int shm_fd_obsrec = shmget(OBSERVE_KEY, SHMSIZE, 0666);
    void * shm_obsrec_addr = shmat(shm_fd_obsrec, NULL, 0);

    // also open shared memory that we initialized in tapper between reconstruct and tapplot
    int shm_fd_rectap = shmget(PLOT_KEY, SHMSIZE, 0666);
    void * shm_rectap_addr = shmat(shm_fd_rectap, NULL, 0);

    if (shm_fd_obsrec == NULL || shm_fd_rectap == NULL) {
        // something went horribly wrong
        perror("shmatt error");
        exit(1);
    }

    // read from shared memory
    char line[SHMSIZE];
    char lastValue[SHMSIZE] = {0};


    

}

