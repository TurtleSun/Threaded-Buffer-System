#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <getopt.h>
#include <time.h>

#define DELAY 5E4 //Delay in microseconds
#define MAX_STRINGS 100 //Maximum number of strings in buffer
#define MAX_STR_LEN 100 //Maximum length of a string in buffer

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

// Creating a brand new buffer and initializing its elements
void createBuffer(key_t key, const char * type, int size) {
    int shmID = shmget(key, sizeof(Buffer), IPC_CREAT | 0666);
    if (shmID == -1) {
        perror("shmget error");
        exit(1);
    }
    void * shmAddr = shmat(shmID, NULL, 0);
    Buffer * buf = (Buffer *)shmAddr;
    buf->in = 0;
    buf->out = 0;
    buf->latest = 0;
    buf->reading = 0;
    buf->slots[0] = 0;
    buf->slots[1] = 0;
    if (strcmp(type, "async") == 0) {
        buf->isAsync = 1;
        buf->size = 4;
    } else if (strcmp(type, "sync") == 0){
        buf->isAsync = 0;
        buf->size = size;
    } else {
        fprintf(stderr, "Invalid buffer type\n");
        exit(1);
    }
}

// Opening an existing buffer
Buffer * openBuffer(key_t key) {
    int shmID = shmget(key, sizeof(Buffer), 0666);
    if (shmID == -1) {
        perror("shmget error");
        exit(1);
    }
    void * shmAddr = shmat(shmID, NULL, 0);
    if (shmAddr == (void*)-1) {
        perror("shmat error");
        exit(1);
    }
    return (Buffer *)shmAddr;
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


void ringWrite (Buffer * buffer, char * item) {
     while (((buffer->in + 1) % buffer->size) == buffer->out);
     strcpy(buffer->data[buffer->in], item);
     buffer->in = (buffer->in + 1) % buffer->size;   
}

char * ringRead (Buffer * buffer) {
    char * result = malloc(100);
    if (result == NULL) {
        perror("malloc error");
        exit(1);
    }

    while (buffer->out == buffer->in);
    strcpy(result, buffer->data[buffer->out]);
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

//Fake main function for testing purposes.
/*int main(int argc, char ** argv) {
    createBuffer(1234, "sync", 10);
    createBuffer(5678, "sync", 10);
    Buffer * myBuf = openBuffer(1234);
    Buffer * myOtherBuf = openBuffer(5678);
    ringWrite(myBuf, "cocaine");
    ringWrite(myBuf, "World");
    char * result = ringRead(myBuf);
    ringWrite(myOtherBuf, result);
    char * newResult = ringRead(myOtherBuf);
    printf("%s", newResult);
    return 0;
}*/