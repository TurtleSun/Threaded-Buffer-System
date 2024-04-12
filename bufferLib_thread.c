#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <getopt.h>

typedef struct {
    // Used for all bufs
    char ** data;
    int size;
    int isAsync;
    //pthread_mutex_t mutex;
    // Used only for ring bufs
    int in;
    int out;
    // Used only for async bufs
    int latest;
    int reading;
    int slots[2];
} Buffer;

typedef struct {
    Buffer buffer;
    int arg3;
    char * fd;
} Parcel;

Parcel initBuffer(char * type, int size, int arg3, char * testFile) {
    Buffer buf;
    buf.in = 0;
    buf.out = 0;
    buf.size = size;
    buf.isAsync = 0;
    buf.latest = 0;
    buf.reading = 0;
    buf.slots[0] = 0;
    buf.slots[1] = 0;
    //pthread_mutex_init(&buf.mutex, NULL);

    // convert type to integer

    printf("\n");
    printf ("I get to initBuffer \n");
    printf ("Here is type: %s\n", type);

    if (strstr(type, "async") != NULL) {
        printf("initBuffer's strcmp worked\n");
        printf("\n");
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

        Parcel parcel = {.buffer = buf, .arg3 = 1, .fd = testFile}; // Just so now, has defaults

    return parcel;
}

void asyncWrite (Buffer * buffer, char * item) {
    int pair, index;
    pair = !buffer->reading;
    index = !buffer->slots[pair]; // Avoids last written slot in this pair, which reader may be reading.
    strncpy(buffer->data[index + (2*pair)], item, 99); // Copy item to data slot. Hardcoded # bytes

    printf("WRITING TO 4-SLOT: %s\n", buffer->data[index + (2*pair)]);

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
    printf("READING FROM 4-SLOT: %s\n", result);
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
        printf("READ BUFF: GOOD\n");
        return asyncRead(buffer);
    } else {
        printf("READ BUFF: BAD\n");
        return ringRead(buffer);
    }
}

void writeBuffer (Buffer * buffer, char * item) {
    if (buffer->isAsync == 1) {
        printf("WRITE BUFF: Am I writing %s\n", item);
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