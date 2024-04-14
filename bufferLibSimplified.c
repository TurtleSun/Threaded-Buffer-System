#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <getopt.h>

#define DELAY 1
#define MAX_STRINGS 100
#define MAX_STR_LEN 100

typedef struct {
    char data[MAX_STRINGS][MAX_STR_LEN];
    int size;
    int in;
    int out;
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
    buf->size = size;
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