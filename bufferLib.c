#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <getopt.h>
#include <semaphore.h>
#include <fcntl.h>

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

void initBuffer(Buffer * buf, const char * type, int size, char * identifier) {
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
        char mutexName[50];
        char slotsEmptyMutexName[50];
        char slotsFullMutexName[50];

        sprintf(mutexName, "%s_mutex", identifier);
        sprintf(slotsEmptyMutexName, "%s_slotsEmptyMutex", identifier);
        sprintf(slotsFullMutexName, "%s_slotsFullMutex", identifier);

        sem_unlink(mutexName);
        sem_unlink(slotsEmptyMutexName);
        sem_unlink(slotsFullMutexName);

        buf->mutex = sem_open(mutexName, O_CREAT, 0644, 1);
        buf->slotsEmptyMutex = sem_open(slotsEmptyMutexName, O_CREAT, 0644, size);
        buf->slotsFullMutex = sem_open(slotsFullMutexName, O_CREAT, 0644, 0);
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

void asyncWrite (Buffer * buffer, char * item) {
  int pair, index;
  pair = !buffer->reading;
  index = !buffer->slots[pair]; // Avoids last written slot in this pair, which reader may be reading.
  strncpy(buffer->data[index + (2*pair)], item, 99); // Copy item to data slot. Hardcoded # bytes
  buffer->slots[pair] = index; // Indicates latest data within selected pair.
  buffer->latest = pair; // Indicates pair containing latest data.
}

char * asyncRead (Buffer * buffer) {
    int pair, index;
    char * result = malloc(100);
    if (result == NULL) {
        fprintf(stderr, "malloc error");
        exit(1);
    }
    pair = buffer->latest; // Get pairing with newest info
    buffer->reading = pair; // Update reading variable to show which pair the reader is reading from.
    index = buffer->slots[pair]; // Get index of pairing to read from
    strncpy(result, buffer->data[index + (2*pair)], 99); // Get desired value from the most recently updated pair.
    return result;
}


void ringWrite (Buffer * buffer, char * item) {
    sem_wait(buffer->mutex);
    sem_wait(buffer->slotsEmptyMutex);

    /* put value into the buffer */
    strncpy(buffer->data[buffer->in], item, 99);
    buffer->in = (buffer->in + 1) % buffer->size;

    sem_post(buffer->mutex);
    sem_post(buffer->slotsFullMutex);
}

char * ringRead (Buffer * buffer) {
    sem_wait(buffer->mutex);
    sem_wait(buffer->slotsFullMutex);

    char * result = malloc(100);
    if (result == NULL) {
        fprintf(stderr, "malloc error");
        exit(1);
    }
    strncpy(result, buffer->data[buffer->out], 99);
    buffer->out = (buffer->out + 1) % buffer->size;

    sem_post(buffer->mutex);
    sem_post(buffer->slotsEmptyMutex);
    return result;
}

char * readBuffer (Buffer * buffer) {
    if (buffer->isAsync == 1) {
        return asyncRead(buffer);
    } else {
        return ringRead(buffer);
    }
}

void writeBuffer (Buffer * buffer, char * item) {
    if (buffer->isAsync == 1) {
        asyncWrite(buffer, item);
    } else {
        ringWrite(buffer, item);
    }
}

//Fake main function for testing purposes.
/*int main(int argc, char ** argv) {
    Buffer myBuf = initBuffer(argc, argv);
    writeBuffer(&myBuf, "hello");
    writeBuffer(&myBuf, "World!");
    char * result = readBuffer(&myBuf);
    printf("%s", result);
    return 0;
}*/