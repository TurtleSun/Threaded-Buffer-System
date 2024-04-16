#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <getopt.h>
#include <pthread.h>

#define DELAY 5E4
#define MAX_STRING_LINES 100
#define MAX_STRINGS 100

typedef struct {
    // Used for all bufs
    char data[MAX_STRINGS][MAX_STRING_LINES];
    int size;
    int isAsync;
    int argn;
    // Used only for ring bufs
    int in;
    int out;
    // Used only for async bufs
    int latest;
    int reading;
    int slots[2];
} Buffer;

typedef struct {
    Buffer *readBuffer;
    Buffer *writeBuffer;
} Parcel;

void initBuffer(char * type, int size, int argn, char * testFile, Buffer * buf) {
    buf->in = 0;
    buf->out = 0;
    buf->size = size;
    buf->isAsync = 0;
    buf->latest = 0;
    buf->reading = 0;
    buf->slots[0] = 0;
    buf->slots[1] = 0;    
    buf->argn = argn;

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

}

void initParcel(Buffer* readBuffer, Buffer* writeBuffer, Parcel * parcel){
    parcel->readBuffer = readBuffer;
    parcel->writeBuffer = writeBuffer;
}

void asyncWrite (Buffer * buffer, char * item) {
  int pair, index;
  pair = !buffer->reading;
  index = !buffer->slots[pair]; // Avoids last written slot in this pair, which reader may be reading.
  strncpy(buffer->data[index + (2*pair)], item, 99); // Copy item to data slot. Hardcoded # bytes
  buffer->slots[pair] = index; // Indicates latest data within selected pair.
  buffer->latest = pair; // Indicates pair containing latest data.
  usleep(DELAY);
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
    usleep(DELAY);
    return result;
}

void ringWrite(Buffer *buffer, char *item) {
    while (((buffer->in + 1) % buffer->size) == buffer->out);
    // Put value into the buffer
    strncpy(buffer->data[buffer->in], item, 99);
    buffer->in = (buffer->in + 1) % buffer->size;
}

char *ringRead(Buffer *buffer) {
    char *result = malloc(100);
    if (result == NULL) {
        fprintf(stderr, "malloc error");
        exit(1);
    }
    while (buffer->out == buffer->in);
    strncpy(result, buffer->data[buffer->out], 99);
    buffer->out = (buffer->out + 1) % buffer->size;
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