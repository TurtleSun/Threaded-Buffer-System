#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "bufferLib_thread.h"
#include <unistd.h>
#include <pthread.h>

#define MAX_PAIRS 100
#define MAX_NAME_LEN 50
#define MAX_VALUE_LEN 50
#define MAX_LINE_LENGTH 100
#define MAX_SAMPLE_SIZE 1024
#define MAX_UNIQUE_NAMES 50

typedef struct {
    char name[MAX_NAME_LEN];
    char value[MAX_VALUE_LEN];
    // extra field to count appearances of each pair
    int count;
} Pair;

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

void *reconstruct_function(void *arg){
    Parcel *arguemnts = (Parcel *)arg;
    Buffer *buffer1 = arguemnts->readBuffer;
    Buffer *buffer2 = arguemnts->writeBuffer;

    KnownValues knownValues = {0};
    // char array to hold the data
    char data[MAX_LINE_LENGTH];
    // initialize a pair to hold the parsed data
    Pair parsedData;

    while (1){
        //fprintf(stderr, "IM TRYING TO READ\n");
        char* dataObs = readBuffer(buffer1);
        //fprintf(stderr, "I READ SOMETHING\n");
        // check for END marker symbolizing no more data to read
        if (strcmp(dataObs, "END_OF_DATA") == 0) {
            break;
        }
        if (dataObs != NULL && (strcmp(dataObs, "") != 0)) {
            parseData(dataObs, &parsedData);

            if (strcmp(knownValues.endName, "") == 0 && nameInKnownVals(&knownValues, parsedData.name) == 1) {
                char sample[100];
                compileSample(&knownValues, sample);
                writeBuffer(buffer2, sample);
            }
            updateLastKnownValues(&parsedData, &knownValues);
            // if we have found the end name, compile the sample
            if (strcmp(parsedData.name, knownValues.endName) == 0) {
                char sample[100];
                compileSample(&knownValues, sample);
                writeBuffer(buffer2, sample);
            }
        }
    }

    // write into the buffer, at the very end, the end marker
    // end of data yeet bc i dont think this will be part of the values we are observing
    writeBuffer(buffer2, "END_OF_DATA");

    pthread_exit(NULL);
}

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
    for (int i = 0; i < knownValues->count; ++i) {
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