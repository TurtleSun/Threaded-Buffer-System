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
#define KEY 1234
#define SHMSIZE 100000

int main(){
    // open shared memory that we initialized in tapper
    int shm_fd = shmget(KEY, SHMSIZE, 0666);
    char * shm_addr = shmat(shm_fd, NULL, 0);
    if (shm_addr == NULL) {
        // something still went horribly wrong
        perror("malloc error");
        exit(1);
    }

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
            // Write to shared memory
            snprintf(shm_addr, MAX_LINE_LEN, "%s=%s", name, value);
        }
    }

    // close shared memory
    close(shm_fd);

    // exit
    return 0;
}