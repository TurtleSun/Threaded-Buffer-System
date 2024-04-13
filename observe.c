// P1's task is to observe input as it arrives and report to the next process any changes it sees in the input
// discarding any previously observed "name=value\n" strings.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include "bufferLib.h"

//  reads input either from a file or stdin. The input data will be a series of strings, each terminated by a newline either until the end-of-file input or the user types ctrl-d to end the standard input stream. NOTE that ctrl-d (ctrl  and the 'd' key together) sends an EOF signal to the foreground process, so as long as tapper is running, it will not terminate your shell program. 
// Each string input to P1 will be of the form:
// "name=value\n" -- where both name and value could be multiple whitespace delimited groups of characters. For example: "temperature=60 Fahrenheit\n", "time=102 milliseconds\n", or "sample reading=102\n". 

#define MAX_NAME_LEN 100
#define MAX_VALUE_LEN 100
#define MAX_LINE_LEN 200
#define KEY 1234
            /// should this be buffer_size?
#define SHMSIZE 100000

struct Pair {
    char * name;
    char * value;
};

struct PairList {
    struct Pair pairs[10];
    int numPairs;
};

struct PairList pairlist = { .pairs = {0}, .numPairs = 0 };

struct PairList updateLastKnown(struct PairList pairlist, char * name, char * newVal) {
    for (int i = 0; i < pairlist.numPairs; i++) {
        if (strcmp(pairlist.pairs[i].name, name) == 0 && strcmp(pairlist.pairs[i].value, newVal) != 0) {
            strcpy(pairlist.pairs[i].value, newVal);
            return pairlist;
        }
    }
    strcpy(pairlist.pairs[pairlist.numPairs].name, name);
    strcpy(pairlist.pairs[pairlist.numPairs].value, newVal);
    pairlist.numPairs++;
    return pairlist;
}

char * getLastKnown(struct PairList pairlist, char * name) {
    for (int i = 0; i < pairlist.numPairs; i++) {
        if (strcmp(pairlist.pairs[i].name, name) == 0) {
            return pairlist.pairs[i].value;
        }
    }
    return "";
}

int main(int argc, char *argv[]) {
    // open shared memory that we initialized in tapper
    for (int i = 0; i < 10; i++) {
        pairlist.pairs[i].name = malloc(100);
        pairlist.pairs[i].value = malloc(100);
        if (pairlist.pairs[i].name == NULL || pairlist.pairs[i].value == NULL) {
            fprintf(stderr, "malloc error");
            exit(1);
        }
    }
    Buffer * shmBuffer = openBuffer(KEY, SHMSIZE);

    // read and parse from either a file or stdin
    char line[MAX_LINE_LEN];
    char lastValue[MAX_VALUE_LEN] = {0};

    // loop to read from stdin
    while (fgets(line, MAX_LINE_LEN, stdin) != NULL){
        fprintf(stderr, "OBSERVE GOT LINE %s\n", line);
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

        // print line

        // compare current value with last value, if they are different write value in shared memory
        if(strcmp(getLastKnown(pairlist, name), value) != 0) {
            strcpy(lastValue, value);
            pairlist = updateLastKnown(pairlist, name, value);
            // Write to shared memory
            //TODO: Change shm_addr here to an attribute (slot) of a structure that we cast shm_addr to.
            // snprintf(shm_addr, MAX_LINE_LEN, "%s=%s", name, value);

            // Prepare the string to write into the buffer
            char bufferData[MAX_LINE_LEN];
            snprintf(bufferData, sizeof(bufferData), "%s=%s", name, value);
            
            // Write into the buffer based on its type
            fprintf(stderr, "OBSERVE - Writing to buffer: %s\n", bufferData);
            writeBuffer(shmBuffer, bufferData);
            printf("OBSERVE - Wrote to buffer: %s\n", bufferData);
            // print whats inside
        }
    }
    // once it is done writing data to the buffer, set the reading flag to 1
    shmBuffer->reading = 1;
    // write into the buffer, at the very end, the end marker
    // end of data yeet bc i dont think this will be part of the values we are observing
    writeBuffer(shmBuffer, "END_OF_DATA_YEET");

    // close shared memory
    // detach it
    shmdt(shmBuffer);

    // exit
    fprintf(stderr, "OBSERVE RETS\n");
    fflush(stderr);
    return 0;
}