// P2's task is to execute a program called reconstruct, which reconstructs the missing data not sent from P1. 
// Using the above example, since P2 receives "temperature=60 F",  followed by "time=10 s", then "time=20 s" it knows that 
// "temperature=60 F" is the last such "name=value" pair at "time=20 s". 
// P2's reconstruction process will create a table of samples for all "name=value" pairs seen so far.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

// definitions


int main(){
    // open shared memory that we initialized in tapper between observe and reconstruct
    int shm_fd_obsrec = shm_open(SHM_NAME, O_RDWR, 0666);
    char* shm_ptr_obsrec = mmap(NULL, MAX_LINE_LEN, PROT_READ, MAP_SHARED, shm_fd_obsrec, 0);

    // also open shared memory that we initialized in tapper between reconstruct and tapplot
    int shm_fd_rectap = shm_open(SHM_NAME, O_RDWR, 0666);
    char* shm_ptr_rectap = mmap(NULL, MAX_SAMPLES * MAX_SAMPLE_SIZE, PROT_WRITE, MAP_SHARED, shm_fd_rectap, 0);

    // open existing semaphores
    sem_t *sem_observe_ready = sem_open(SEM_OBSERVE, 0);
    sem_t *sem_tapplot_ready = sem_open(SEM_TAPPLOT, O_CREAT, 0666, 0);
    sem_t *sem_mutex_observe = sem_open(SEM_MUTEX_OBSERVE, 0);
    sem_t *sem_mutex_tapplot = sem_open(SEM_MUTEX_TAPPLOT, O_CREAT, 0666, 1);

    // check if shared memory is opened successfully
    if (shm_ptr_obsrec == MAP_FAILED || shm_ptr_rectap == MAP_FAILED){
        printf("Map failed\n");
        return -1;
    }

    // read from shared memory
    

}

