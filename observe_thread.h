#define OBSERVE_THREAD_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
//#include "bufferLib_thread.h"

struct Pair {
    char * name;
    char * value;
};

struct PairList {
    struct Pair pairs[10];
    int numPairs;
};

//struct PairList updateLastKnown(struct PairList pairlist, char * name, char * newVal);
//char * getLastKnown(struct PairList pairlist, char * name);
void *observe_function(void* arg);
