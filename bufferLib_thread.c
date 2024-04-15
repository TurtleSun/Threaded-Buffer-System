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

    printf("Entered initBuffer\n");
    buf->in = 0;
    buf->out = 0;
    buf->size = size;
    buf->isAsync = 0;
    buf->latest = 0;
    buf->reading = 0;
    buf->slots[0] = 0;
    buf->slots[1] = 0;    
    buf->argn = argn;


    // convert type to integer

    //printf("\n");
    printf ("I get to initBuffer \n");
    //printf ("Here is type: %s\n", type);

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

    printf ("Parsed and initated buf.isAsync: %d and buf.size: %d \n", buf->isAsync, buf->size);

/*     // Allocate data pointers
    buf->data = (char **)malloc(sizeof(char *) * buf->size);
    if (!buf->data) {
        perror("malloc for data pointers failed");
        exit(EXIT_FAILURE);
    }

    // Allocate each string in the buffer
    for (int i = 0; i < buf->size; i++) {
        buf->data[i] = malloc(100);
        if (buf->data[i] == NULL) {
            fprintf(stderr, "malloc error");
            exit(1);
        }
    } */

/*     Buffer buf;
    buf.in = 0;
    buf.out = 0;
    buf.size = size;
    buf.isAsync = 0;
    buf.latest = 0;
    buf.reading = 0;
    buf.slots[0] = 0;
    buf.slots[1] = 0;

    // convert type to integer

    //printf("\n");
    //printf ("I get to initBuffer \n");
    //printf ("Here is type: %s\n", type);

    if (strcmp(type, "async") == 0) {
        buf.isAsync = 1;
        buf.size = 4;
    } else if (strcmp(type, "sync") == 0) {
        buf.isAsync = 0;
        if (size <= 0) {
            fprintf(stderr, "Invalid size for ring buffer!\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Invalid type of buffer!\n");
        exit(1);
    }

    printf ("Parsed and initated buf.isAsync: %d and buf.size: %d \n", buf.isAsync, buf.size);

    // Allocate data pointers
    buf.data = (char **)malloc(sizeof(char *) * buf.size);
    if (!buf.data) {
        perror("malloc for data pointers failed");
        exit(EXIT_FAILURE);
    }

    // Allocate each string in the buffer
    for (int i = 0; i < buf.size; i++) {
        buf.data[i] = malloc(100);
        if (buf.data[i] == NULL) {
            fprintf(stderr, "malloc error");
            exit(1);
        }
    }

    printf("Initated and allocated buf data\n");

    if (pthread_mutex_init(&buf.mutex, NULL) != 0) {
        perror("pthread_mutex_init failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&buf.slotsEmptyCond, NULL) != 0 ||
        pthread_cond_init(&buf.slotsFullCond, NULL) != 0) {
        perror("pthread_cond_init failed");
        exit(EXIT_FAILURE);
    } */

}

void initParcel(Buffer* readBuffer, Buffer* writeBuffer, Parcel * parcel){
    parcel->readBuffer = readBuffer;
    parcel->writeBuffer = writeBuffer;
}

void asyncWrite (Buffer * buffer, char * item) {
  fprintf (stderr,"ASYNCWRITE ITEM PASSED: %s\n", item);
  int pair, index;
  pair = !buffer->reading;
  index = !buffer->slots[pair]; // Avoids last written slot in this pair, which reader may be reading.
  fprintf(stderr, "ITEM BEFORE: %s\n", item);
  fprintf(stderr, "BUFFER BEFORE: %s\n", buffer->data[index + (2*pair)]);
  strncpy(buffer->data[index + (2*pair)], item, 99); // Copy item to data slot. Hardcoded # bytes
  fprintf(stderr, "ITEM AFTER: %s\n", item);
  fprintf(stderr, "BUFFER AFTER : %s\n", buffer->data[index + (2*pair)]);
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
    fprintf(stderr, "STARTING RING WRITE\n");

    fprintf(stderr, "RING: PASSED MUTEX, NOW WRITING\n");
    // Put value into the buffer
    strncpy(buffer->data[buffer->in], item, 99);
    buffer->in = (buffer->in + 1) % buffer->size;

    fprintf(stderr, "FIN RING WRITE\n");
}

char *ringRead(Buffer *buffer) {
    
    char *result = malloc(100);
    if (result == NULL) {
        fprintf(stderr, "malloc error");
        exit(1);
    }
    strncpy(result, buffer->data[buffer->out], 99);
    buffer->out = (buffer->out + 1) % buffer->size;
    
    return result;
}

char * readBuffer (Buffer * buffer) {
    if (buffer->isAsync == 1) {
        //printf("READ BUFF: GOOD\n");
        return asyncRead(buffer);
    } else {
        //printf("READ BUFF: BAD\n");
        return ringRead(buffer);
    }
}

void writeBuffer (Buffer * buffer, char * item) {
    if (buffer->isAsync == 1) {
        //printf("WRITE BUFF: Am I writing %s\n", item);
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