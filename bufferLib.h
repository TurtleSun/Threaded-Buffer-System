#ifndef BUFFERLIB
#include <semaphore.h>
#define BUFFERLIB .H

typedef struct {
    // Used for all bufs
    char ** data;
    int size;
    int isAsync;
    // Used only for ring bufs
    int in;
    int out;
    sem_t * mutex;
    sem_t * slotsEmptyMutex;
    sem_t * slotsFullMutex;
    // Used only for async bufs
    int latest;
    int reading;
    int slots[2];
} Buffer;

//init
void initBuffer(Buffer * buf, const char * type, int size, char * identifier);

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