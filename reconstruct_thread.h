#define RECONSTRUCT_THREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
//#include "bufferLib_thread.h"
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
void *reconstruct_function(void *arg);
