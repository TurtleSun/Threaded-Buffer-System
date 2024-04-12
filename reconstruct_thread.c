#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "bufferLib_thread.h"
#include <unistd.h>

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

typedef struct {
    // holds pairs for a single sample
    Pair pairs[MAX_UNIQUE_NAMES];
    // number of pairs in this sample
    int pairCount;
} Sample;

typedef struct {
    // array to hold samples
    Sample samples[MAX_SAMPLE_SIZE];
    // current number of samples
    int sampleCount;
} Samples;

// Function declarations
void parseData(const char* data, Pair* outPair);
void updateLastKnownValues(Pair* newPair, KnownValues* knownValues);
int shouldCompileSample(Pair* newPair, KnownValues* knownValues);
void findEndName(KnownValues *values);
void compileSample(KnownValues* knownValues, char* outSample);
void addSample(Samples* samples, KnownValues* knownValues);

void *reconstruct_function(void *arg){
    printf("Reconstruct thread made!");
    Parcel *arguemnts = (Parcel *)arg;
    Buffer buffer = arguemnts->buffer;

    //printf("Reconstruct BUFF: %p\n", &buffer);
    //printf("Reconstruct BUFF ISASYNC: %d\n", buffer.isAsync);

    // initialize known values struct to hold the last known values
    KnownValues knownValues = {0};
    // char array to hold the data
    char data[MAX_LINE_LENGTH];
    // initialize a pair to hold the parsed data
    Pair parsedData;

    ///////////////////// READING FROM OBSERVE
    // read from shared memory between observe and reconstruct and reconstruct the data logic in separate function
    // while reading flag is 0 then the data is not ready to read
    while (!buffer.reading) {
        // sleep briefly
        usleep(1000);
    }

    Samples samples = {0};

    // reading from the buffer 
    // process data from obsrecBuffer, data observe wrote
    while (1){
        char* dataObs = readBuffer(&buffer);
        // check for END marker symbolizing no more data to read
        if (strcmp(dataObs, "END_OF_DATA") == 0) {
            break;
        }
        if (dataObs != NULL) {
            parseData(dataObs, &parsedData);
            updateLastKnownValues(&parsedData, &knownValues);
            findEndName(&knownValues);
            // if we have found the end name, compile the sample
            if (strcmp(parsedData.name, knownValues.endName) == 0) {
                char sample[MAX_SAMPLE_SIZE];
                compileSample(&knownValues, sample);
                printf("There is the sample: %s\n", sample);  
                writeBuffer(&buffer, sample);
            }
        }
    }

    // once it is done writing data to the buffer, set the reading flag to 1
    buffer.reading = 1;
    // write into the buffer, at the very end, the end marker
    // end of data yeet bc i dont think this will be part of the values we are observing
    writeBuffer(&buffer, "END_OF_DATA");

    pthread_exit(NULL);
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

// isEndName function
// checks if the name is the end name
int isEndName(const char* name, KnownValues *values) {
    return strcmp(name, values->endName) == 0;
}

// compileSample function
// compiles the sample from the known values and end name


// addSample function
// adds a sample to the samples struct
void addSample(Samples* samples, KnownValues* knownValues) {
    if (samples->sampleCount >= MAX_SAMPLE_SIZE) {
        // No more space for samples
        return;
    }
    
    Sample* newSample = &samples->samples[samples->sampleCount++];
    // initialize the pair count for the new sample
    newSample->pairCount = 0;
    
    // Copy data from knownValues to the newSample
    for (int i = 0; i < knownValues->count; i++) {
        strcpy(newSample->pairs[i].name, knownValues->pairs[i].name);
        strcpy(newSample->pairs[i].value, knownValues->pairs[i].value);
        newSample->pairCount++;
    }
}

// shouldCompileSample function
// checks if the sample should be compiled
int shouldCompileSample(Pair* newPair, KnownValues* knownValues) {
    // if the new pair is the end name, compile the sample
    if (isEndName(newPair->name, knownValues)) {
        return 1;
    }
    return 0;
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