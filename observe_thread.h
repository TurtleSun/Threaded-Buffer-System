#define OBSERVE_THREAD .H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct Pair {
    char * name;
    char * value;
};

struct PairList {
    struct Pair pairs[10];
    int numPairs;
};

void *observe_function(void* arg);