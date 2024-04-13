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

void initBuffer(Buffer * buf, const char * type, int size) {
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
    } else {
        fprintf(stderr, "Invalid type of buffer!\n");
        exit(1);
    }

    // Allocate data pointers in shared memory
    buf->data = (char**)(buf + 1);  // The data array starts immediately after the Buffer struct

    // Allocate each string in the buffer
    for (int i = 0; i < buf->size; i++) {
        buf->data[i] = (char *)(buf->data + buf->size) + i * 100;  // Each string is 100 bytes
    }
}

// Function declarations
void parseData(const char* data, Pair* outPair);
void updateLastKnownValues(Pair* newPair, KnownValues* knownValues);
void findEndName(KnownValues *values);
void compileSample(KnownValues* knownValues, char* outSample);

int main(int argc, char *argv[]) {
    // open shared memory that we initialized in tapper between observe and reconstruct
    int shm_Id_obsrec = shmget(OBSERVE_KEY, SHMSIZE, 0666);
    void * shm_obsrec_addr = shmat(shm_Id_obsrec, NULL, 0);

    // also open shared memory that we initialized in tapper between reconstruct and tapplot
    int shm_Id_rectap = shmget(PLOT_KEY, SHMSIZE, 0666);
    void * shm_rectap_addr = shmat(shm_Id_rectap, NULL, 0);

    if (shm_Id_obsrec == -1 || shm_Id_rectap == -1) {
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

    // initialize known values struct to hold the last known values
    KnownValues knownValues = {0};
    // char array to hold the data
    char data[MAX_LINE_LENGTH];
    // initialize a pair to hold the parsed data
    Pair parsedData;

    // Initialize the buffer with received type and size
    initBuffer(obsrecBuffer, bufferType, bufferSize);
    initBuffer(rectapBuffer, bufferType, bufferSize);

    ///////////////////// READING FROM OBSERVE
    // read from shared memory between observe and reconstruct and reconstruct the data logic in separate function
    // while reading flag is 0 then the data is not ready to read

   // while (!obsrecBuffer->reading) {
     //   // sleep briefly
       // usleep(1000);
    //}

    // reading from the buffer 
    // process data from obsrecBuffer, data observe wrote
    while (true){
        printf("Reading data from obsrecBuffer\n");
        char* dataObs = readBuffer(obsrecBuffer);
        printf("YULU");
        // check for END marker symbolizing no more data to read
        if (strcmp(dataObs, "END_OF_DATA_YEET") == 0) {
            break;
        }
        printf("Data: %s\n", dataObs);
        if (dataObs != NULL) {
            parseData(dataObs, &parsedData);
            // print values
            printf("Name: %s, Value: %s\n", parsedData.name, parsedData.value);
            updateLastKnownValues(&parsedData, &knownValues);
            findEndName(&knownValues);
            // if we have found the end name, compile the sample
            if (strcmp(parsedData.name, knownValues.endName) == 0) {
                printf("End name found\n");
                char sample[MAX_SAMPLE_SIZE];
                compileSample(&knownValues, sample);
                printf("There is the sample: %s\n", sample);  
                writeBuffer(rectapBuffer, sample);
            }
        }
    }

    // once it is done writing data to the buffer, set the reading flag to 1
    //rectapBuffer->reading = 1;
    // write into the buffer, at the very end, the end marker
    // end of data yeet bc i dont think this will be part of the values we are observing
    writeBuffer(rectapBuffer, "END_OF_DATA_YEET");

    // Detach from shared memory segments
    shmdt(shm_obsrec_addr);
    shmdt(shm_rectap_addr);

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
        Pair selectedPair = knownValues->pairs[knownValues->count];
        strcpy(selectedPair.name, newPair->name);
        strcpy(selectedPair.value, newPair->value);
        knownValues->pairs[knownValues->count].count = 1;
        knownValues->count++;
    }
}

// findEndName function
// checks if the name is the end name
// this way we can check when a sample is "completed" to be compiled
void findEndName(KnownValues *values) {
    // go through the values in KnownValues
    // find the first value whose count is nonzer (repeated)
    // end value is the name before it
    if (values->count > 0) {
        strcpy(values->endName, values->pairs[values->count - 1].name);
    }

}

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
