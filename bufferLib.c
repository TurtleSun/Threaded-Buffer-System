#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <getopt.h>

typedef struct {
    // Used for all bufs
    char ** data;
    int in;
    int out;
    int size;
    int isAsync;
    // Used only for async bufs
    int latest;
    int reading;
    int slots[2];
} Buffer;

Buffer initBuffer(int argc, char ** argv) {
    Buffer buf;
    buf.in = 0;
    buf.out = 0;
    buf.size = 0;
    buf.isAsync = 0;
    buf.latest = 0;
    buf.reading = 0;
    buf.slots[0] = 0;
    buf.slots[1] = 0;

    int opt;
    char type[1024];
    while ((opt = getopt(argc, argv, "b:s:")) != -1) {
        switch(opt) {
            case 'b':
                strcpy(type, optarg);
                break;
            case 's':
                buf.size = atoi(optarg);
                break;
            case '?':
                fprintf(stderr, "getopt error - missing/bad argument?");
                exit(1);
        }
    }

    if (strcmp(type, "async") == 0) {
        buf.size = 4;
    } else if (strcmp(type, "sync") == 0 && buf.size == 0) {
        fprintf(stderr, "Invalid size for ring buffer!");
        exit(1);
    } else if (strcmp(type, "async") != 0 && strcmp(type, "sync") != 0) {
        fprintf(stderr, "Invalid type of buffer!");
        exit(1);
    }

    buf.data = malloc(sizeof(char * ) * buf.size);
    if (buf.data == NULL) {
        fprintf(stderr, "malloc error");
        exit(1);
    }
    for (int i = 0; i < buf.size; i++) {
        buf.data[i] = malloc(100);
        if (buf.data[i] == NULL) {
            fprintf(stderr, "malloc error");
            exit(1);
        }
    }
    return buf;
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
     while (((buffer->in + 1) % buffer->size) == buffer->out);
     /* put value into the buffer */
     strncpy(buffer->data[buffer->in], item, 99);
     buffer->in = (buffer->in + 1) % buffer->size; 
}

char * ringRead (Buffer * buffer) {
    while (buffer->out == buffer->in);
    /* take one unit of data from the buffer */
    char * tmp = malloc(100);
    if (tmp == NULL) {
        fprintf(stderr, "malloc error");
        exit(1);
    }
    strncpy(tmp, buffer->data[buffer->out], 99);
    buffer->out = (buffer->out + 1) % buffer->size;
    return tmp;
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
    writeBuffer(&myBuf, "shit");
    char * result = readBuffer(&myBuf);
    return 0;
}*/