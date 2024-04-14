#ifndef BUFFERLIB
#include <semaphore.h>
#define BUFFERLIB .H

typedef struct {
    char ** data;
    int size;
    int in;
    int out;
} Buffer;

//init
void createBuffer(key_t key, const char * type, int size);
Buffer * openBuffer(key_t key);

void ringWrite (Buffer * buffer, char * item);
char * ringRead (Buffer * buffer);

#endif