#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "bufferLib_thread.h"
#include <pthread.h>

//  reads input either from a file or stdin. The input data will be a series of strings, each terminated by a newline either until the end-of-file input or the user types ctrl-d to end the standard input stream. NOTE that ctrl-d (ctrl  and the 'd' key together) sends an EOF signal to the foreground process, so as long as tapper is running, it will not terminate your shell program. 
// Each string input to P1 will be of the form:
// "name=value\n" -- where both name and value could be multiple whitespace delimited groups of characters. For example: "temperature=60 Fahrenheit\n", "time=102 milliseconds\n", or "sample reading=102\n". 

#define MAX_NAME_LEN 100
#define MAX_VALUE_LEN 100
#define MAX_LINE_LEN 200

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

void *observe_function(void* arg) {
    printf("Observe thread made!\n");

    for (int i = 0; i < 10; i++) {
        pairlist.pairs[i].name = malloc(100);
        pairlist.pairs[i].value = malloc(100);
        if (pairlist.pairs[i].name == NULL || pairlist.pairs[i].value == NULL) {
            fprintf(stderr, "malloc error");
            exit(1);
        }
    }

    Parcel *arguemnts = (Parcel *)arg;
    printf("arguemnts isAsync = %d\n", arguemnts->buffer->isAsync);
    Buffer *buffer = arguemnts->buffer;
    char * testFile = arguemnts->fd;

    //printf("Observe BUFF: %p\n", &buffer);
    printf("CHECKING: Observe BUFF ISASYNC: %d\n", buffer->isAsync);
    printf("CHECKING: Observe BUFF TESTFILE: %s\n", arguemnts->fd);

    FILE *fp;
    if(testFile != "standardOut") {
        fp = fopen(testFile, "r");
    }else {
        fp = stdin;
    }

    printf("OBSRVE: CHECKING FILE OPENED");

    // read and parse from either a file or stdin
    char line[MAX_LINE_LEN];
    char lastValue[MAX_VALUE_LEN] = {0};

    // loop to read from stdin
    while (fgets(line, MAX_LINE_LEN, fp) != NULL){
        // parse the line
        // remove newline character
        line[strcspn(line, "\n")] = 0;
        // split the line into name and value
        char* name = strtok(line, "=");
        printf("JUST STRTOKED (name): %s\n", name);
        char* value = strtok(NULL, "");
        printf("JUST STRTOKED (value): %s\n", value);

        // if the line is malformed
        if (name == NULL || value == NULL){
            printf("Malformed input: %s\n", line);
            continue;
        }

        // print line
        printf("name: %s, value: %s\n", name, value);

        // compare current value with last value, if they are different write value in shared memory
        if(strcmp(getLastKnown(pairlist, name), value) != 0) {
            printf("Value changed: %s\n", value);
            strcpy(lastValue, value);
            pairlist = updateLastKnown(pairlist, name, value);
            // Write to shared memory
            //TODO: Change shm_addr here to an attribute (slot) of a structure that we cast shm_addr to.
            // snprintf(shm_addr, MAX_LINE_LEN, "%s=%s", name, value);

            // Prepare the string to write into the buffer
            char bufferData[MAX_LINE_LEN];
            snprintf(bufferData, sizeof(bufferData), "%s=%s", name, value);
            
            printf("bufferData: %s\n", bufferData);
            // Write into the buffer based on its type
            writeBuffer(buffer, bufferData);
            // print whats inside
            printf("OBSERVE : WRITTEN TO BUFFER: %s\n", buffer->data[0]);
        }
    }

    writeBuffer(buffer, "END_OF_DATA");

    if (testFile != NULL){
        fclose(fp);
    }

    pthread_exit(NULL);
}