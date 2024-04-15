#ifndef BUFFERLIBSIMPLIFIED
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <getopt.h>
#include <time.h>
#define BUFFERLIBSIMPLIFIED .H
#define MAX_STRINGS 100
#define MAX_STR_LEN 100

typedef struct {
    char data[MAX_STRINGS][MAX_STR_LEN];
    int isAsync;
    int size;
    // Used only for ring bufs
    int in;
    int out;
    // Used only for async bufs
    int latest;
    int reading;
    int slots[2];

} Buffer;

//init
void createBuffer(key_t key, const char * type, int size);
Buffer * openBuffer(key_t key);

//writes
void asyncWrite (Buffer * buffer, char * item);
void ringWrite (Buffer * buffer, char * item);

//reads
char * asyncRead (Buffer * buffer);
char * ringRead (Buffer * buffer);

//wrapper funcs
char * readBuffer (Buffer * buffer);
void writeBuffer (Buffer * buffer, char * item);

#endif