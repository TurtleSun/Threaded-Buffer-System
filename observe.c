// P1's task is to observe input as it arrives and report to the next process any changes it sees in the input
// discarding any previously observed "name=value\n" strings.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//  reads input either from a file or stdin. The input data will be a series of strings, each terminated by a newline either until the end-of-file input or the user types ctrl-d to end the standard input stream. NOTE that ctrl-d (ctrl  and the 'd' key together) sends an EOF signal to the foreground process, so as long as tapper is running, it will not terminate your shell program. 
// Each string input to P1 will be of the form:
// "name=value\n" -- where both name and value could be multiple whitespace delimited groups of characters. For example: "temperature=60 Fahrenheit\n", "time=102 milliseconds\n", or "sample reading=102\n". 

#define MAX_NAME_LEN 100
#define MAX_VALUE_LEN 100
#define MAX_LINE_LEN 200

int main(){
    // open shared memory that we initialized in tapper
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    void* shm_ptr = mmap(0, MAX_LINE_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // check if shared memory is opened successfully
    if (shm_ptr == MAP_FAILED){
        printf("Map failed\n");
        return -1;
    }

    // open existing semaphores
    // These semaphores are used for signaling when data is ready to be read from shared memory and for mutual exclusion access to the shared memory, respectively.
    sem_t* sem_data_ready = sem_open(SEM_NAME_DATA_READY, 0);
    sem_t* sem_mutex = sem_open(SEM_NAME_MUTEX, 0);
    // they have read permission
    // has to be initilized in tapper.c

    // read and parse from either a file or stdin
    char line[MAX_LINE_LEN];
    char lastValue[MAX_VALUE_LEN] = {0};

    // loop to read from stdin
    while (fgets(line, MAX_LINE_LEN, stdin) != NULL){
        // parse the line
        // remove newline character
        line[strcspn(line, "\n")] = 0;
        // split the line into name and value
        char* name = strtok(line, "=");
        char* value = strtok(NULL, "");

        // if the line is malformed
        if (name == NULL || value == NULL){
            printf("Malformed input: %s\n", line);
            continue;
        }

        // compare current value with last value, if they are different write value in shared memory
        if(strcmp(lastValue, value) != 0) {
            strcpy(lastValue, value);
            // Wait for access to shared memory
            sem_wait(sem_mutex); 
            // Write to shared memory
            snprintf(shm_ptr, MAX_LINE_LEN, "%s=%s", name, value);
            // Signal that data is ready
            sem_post(sem_data_ready);
            // Release access to shared memory
            sem_post(sem_mutex);
        }
    }

    // close shared memory
    close(shm_fd);
    // unmap shared memory
    munmap(shm_ptr, MAX_LINE_LEN);
    // close semaphores
    sem_close(sem_data_ready);
    sem_close(sem_mutex);

    // exit
    return 0;
}