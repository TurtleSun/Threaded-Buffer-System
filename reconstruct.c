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
#include <bufferLib.h>

#define OBSERVE_KEY 1234
#define PLOT_KEY 5678
#define SHMSIZE 100000

int main(){
    // open shared memory that we initialized in tapper between observe and reconstruct
    int shm_Id_obsrec = shmget(OBSERVE_KEY, SHMSIZE, 0666);
    void * shm_obsrec_addr = shmat(shm_Id_obsrec, NULL, 0);

    // also open shared memory that we initialized in tapper between reconstruct and tapplot
    int shm_Id_rectap = shmget(PLOT_KEY, SHMSIZE, 0666);
    void * shm_rectap_addr = shmat(shm_Id_rectap, NULL, 0);

    if (shm_Id_obsrec == NULL || shm_Id_rectap == NULL) {
        // something went horribly wrong
        perror("shmatt error");
        exit(1);
    }

    // buffer for shared memory between observe and reconstruct
    Buffer *obsrecBuffer = (Buffer *)shm_obsrec_addr;
    // buffer for shared memory between reconstruct and tapplot
    Buffer *rectapBuffer = (Buffer *)shm_rectap_addr;

    // initialize buffers
    // depending on the buffer type, we need to initialize the buffer differently
    // check from inputs
    // Variables for command line arguments
    char *bufferType = NULL;
    int bufferSize = 0;

    // Parsing command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "b:s:")) != -1) {
        switch (opt) {
            case 'b':
                bufferType = optarg;
                break;
            case 's':
                bufferSize = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-b bufferType] [-s bufferSize]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Initialize the buffer with received type and size
    Buffer obsrecBuffer = initBuffer(bufferType, bufferSize);
    Buffer rectapBuffer = initBuffer(bufferType, bufferSize);

    // read from shared memory between observe and reconstruct and reconstruct the data logic in separate function
    reconstructData(obsrecBuffer, rectapBuffer);

    // Detach from shared memory segments
    shmdt(shm_obsrec_addr);
    shmdt(shm_rectap_addr);

    return 0;

}

void reconstructData(Buffer *obsrecBuffer, Buffer *rectapBuffer) {
    Entry entry;
    while (1) {
        // Simulating a read operation from the buffer
        entry = readFromBuffer(obsrecBuffer); 

        if (strcmp(entry.name, "") == 0 && strcmp(entry.value, "") == 0) {
            // This could be a signal that there's no more data to read, for example
            break;
        }

        // Logic to reconstruct the data based on the unique names and their latest values
        // ...
        // After reconstructing a sample, write it to the buffer for tapplot
        writeToBuffer(rectapBuffer, &entry);
    }
}   

