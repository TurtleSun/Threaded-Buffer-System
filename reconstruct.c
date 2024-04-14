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
#include "bufferLib.h"
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <semaphore.h>

#define MAX_PAIRS 100
#define MAX_NAME_LEN 50
#define MAX_VALUE_LEN 50
#define MAX_LINE_LENGTH 100
#define MAX_SAMPLE_SIZE 1024
#define MAX_UNIQUE_NAMES 50

#define OBSERVE_KEY 1234
#define PLOT_KEY 5678
#define SHMSIZE 100000

#define MAX_SAMPLES 100 // Maximum number of samples

typedef struct {
    char name[MAX_NAME_LEN];
    char value[MAX_VALUE_LEN];
    // extra field to count appearances of each pair
    int count;
} Pair;

// array to hold the last known values
//Pair lastKnownValues[MAX_UNIQUE_NAMES];
//int uniqueNames = 0;

typedef struct {
    Pair pairs[MAX_UNIQUE_NAMES];
    // extra field to hold the count of unique names
    int count;
    // extra field to hold the end name
    char endName[100];
} KnownValues;

// Function declarations
void parseData(const char* data, Pair* outPair);
void updateLastKnownValues(Pair* newPair, KnownValues* knownValues);
void findEndName(KnownValues *values);
void compileSample(KnownValues* knownValues, char* outSample);
int nameInKnownVals(KnownValues *values, char * name);

int main(int argc, char *argv[]) {
    // open shared memory that we initialized in tapper between observe and reconstruct
    Buffer * obsrecBuffer = openBuffer(OBSERVE_KEY, SHMSIZE);
    Buffer * rectapBuffer = openBuffer(PLOT_KEY, SHMSIZE);

    // initialize known values struct to hold the last known values
    KnownValues knownValues = {0};
    // char array to hold the data
    char data[MAX_LINE_LENGTH];
    // initialize a pair to hold the parsed data
    Pair parsedData;

    // Initialize the buffer with received type and size
    //initBuffer(obsrecBuffer, bufferType, bufferSize);
    initBuffer(rectapBuffer, bufferType, bufferSize);

    ///////////////////// READING FROM OBSERVE
    // read from shared memory between observe and reconstruct and reconstruct the data logic in separate function
    // while reading flag is 0 then the data is not ready to read

    // reading from the buffer 
    // process data from obsrecBuffer, data observe wrote
    while (true){
        fprintf(stderr, "IM TRYING TO READ\n");
        char* dataObs = readBuffer(obsrecBuffer);
        fprintf(stderr, "I READ SOMETHING\n");
        // check for END marker symbolizing no more data to read
        if (strcmp(dataObs, "END_OF_DATA_YEET") == 0) {
            break;
        }
        printf("Data: %s\n", dataObs);
        if (dataObs != NULL) {
            parseData(dataObs, &parsedData);
            if (strcmp(knownValues.endName, "") == 0 && nameInKnownVals(&knownValues, parsedData.name) == 1) {
                char sample[100];
                compileSample(&knownValues, sample); 
                writeBuffer(rectapBuffer, sample);
                fprintf(stderr, "RECONSTRUCT - Wrote to buffer: %s\n", sample);
            }
            updateLastKnownValues(&parsedData, &knownValues);
            //findEndName(&knownValues);
            // if we have found the end name, compile the sample
            if (strcmp(parsedData.name, knownValues.endName) == 0) {
                char sample[100];
                compileSample(&knownValues, sample);
                writeBuffer(rectapBuffer, sample);
                fprintf(stderr, "RECONSTRUCT - Wrote to buffer: %s\n", sample);  
            }
        }
    }

    // write into the buffer, at the very end, the end marker
    // end of data yeet bc i dont think this will be part of the values we are observing
    writeBuffer(rectapBuffer, "END_OF_DATA_YEET");
    fprintf(stderr, "RECONSTRUCT - Wrote to buffer: END_OF_DATA_YEET\n");

    // Detach from shared memory segments
    shmdt(obsrecBuffer);
    shmdt(rectapBuffer);

    fprintf(stderr, "RECON RETS\n");
    fflush(stderr);
    return 0;
}

/*void initBuffer(Buffer * buf, const char * type, int size) {
    buf->in = 0;
    buf->out = 0;
    buf->size = size;
    buf->isAsync = 0;
    buf->latest = 0;
    buf->reading = 0;
    buf->slots[0] = 0;
    buf->slots[1] = 0;

    // convert type to integer
    if (strcmp(type, "async") == 0) {
        buf->isAsync = 1;
        buf->size = 4;
    } else if (strcmp(type, "sync") == 0) {
        buf->isAsync = 0;
        if (size <= 0) {
            fprintf(stderr, "Invalid size for ring buffer!\n");
            exit(1);
        }
        buf->mutex = sem_open("mutex", O_CREAT, 0644, 1);
        buf->slotsEmptyMutex = sem_open("slotsEmptyMutex", O_CREAT, 0644, size);
        buf->slotsFullMutex = sem_open("slotsFullMutex", O_CREAT, 0644, 0);
    } else {
        fprintf(stderr, "Invalid type of buffer!\n");
        exit(1);
    }

    // Allocate data pointers in shared memory
    buf->data = (char**)(buf + 1);  // The data array starts immediately after the Buffer struct

    // Allocate each string in the buffer
    for (int i = 0; i < buf->size; i++) {
        buf->data[i] = (char *)buf->data + buf->size * sizeof(char*) + i * 100;  // Each string is 100 bytes
    }
}*/

int nameInKnownVals(KnownValues *myValues, char * name) {
    for (int i = 0; i < myValues->count; i++) {
        if (strcmp(myValues->pairs[i].name, name) == 0) {
            int test = 0;
            strcpy(myValues->endName, myValues->pairs[myValues->count-1].name);
            return 1;
        }
    }
    return 0;
}

// parseData function
// splits a "name=value" string into a Pair struct.
void parseData(const char* data, Pair* outPair) {
    // up to 99 characters for name, up to 99 characters for value, and up to 1 character for the '='
    sscanf(data, "%99[^=]=%99[^\n]", outPair->name, outPair->value);
}

// updateLastKnownValues function
// updates or adds to the list of last known values.
void updateLastKnownValues(Pair* newPair, KnownValues* knownValues) {
    int unique = 0;
    // for each known value
    for (int i = 0; i < knownValues->pairs[i].count; ++i) {
        // if the new name is found in the list of known values, update the value
        if (strcmp(knownValues->pairs[i].name, newPair->name) == 0) {
            strcpy(knownValues->pairs[i].value, newPair->value);
            knownValues->pairs[i].count++;
            unique = 1;
            break;
        }
    }
    // if the name is not found in the list of known values, add it
    if (!unique && knownValues->count < MAX_PAIRS) {
        strcpy(knownValues->pairs[knownValues->count].name, newPair->name);
        strcpy(knownValues->pairs[knownValues->count].value, newPair->value);
        knownValues->pairs[knownValues->count].count = 1;
        knownValues->count++;
    }
}

// findEndName function
// checks if the name is the end name
// this way we can check when a sample is "completed" to be compiled
/*void findEndName(KnownValues *myValues) {
    // go through the values in KnownValues
    // find the first value whose count is nonzer (repeated)
    // end value is the name before it
    for (int i = 0; i < myValues->count; i++) {
        if (myValues->pairs[i].count > 1) {
            strcpy(myValues->endName, myValues->pairs[myValues->count - 1].name);
        }
    }
}*/

// compileSample function
// compiles the sample from the known values and end name
void compileSample(KnownValues *values, char sample[]) {
    // Initialize the sample string
    sample[0] = '\0'; // Ensure the string is empty initially

    // Iterate over the known values and concatenate them into the sample string
    for (int i = 0; i < values->count; ++i) {
        // Concatenate the name-value pair to the sample string
        strcat(sample, values->pairs[i].name);
        strcat(sample, "=");
        strcat(sample, values->pairs[i].value);

        // Add a comma if it's not the last pair
        if (i < values->count - 1) {
            strcat(sample, ", ");
        }
    }
}
