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

#define DELAY 1

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

// Creating a brand new buffer and initializing its elements
void createBuffer(key_t key, int shmsize, const char * type, int size) {
    int shmID = shmget(key, sizeof(Buffer), IPC_CREAT | 0666);
    if (shmID == -1) {
        perror("shmget error");
        exit(1);
    }
    void * shmAddr = shmat(shmID, NULL, 0);
    if (shmAddr == (void *)-1) {
        perror("shmat error");
        exit(1);
    }
    Buffer * buf = (Buffer *)shmAddr;
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

        int mutexID = shmget(key - 1, sizeof(sem_t) * 3, IPC_CREAT | 0666);
        if (mutexID == -1) {
            perror("shmget error");
            exit(1);
        }

        void * mutexAddr = shmat(mutexID, NULL, 0);
        if (mutexAddr == (void *)-1) {
            perror("shmat error");
            exit(1);
        }

        buf->mutex = (sem_t *)mutexAddr;
        buf->slotsEmptyMutex = (sem_t *)((char*)buf->mutex + sizeof(sem_t));
        buf->slotsFullMutex = (sem_t *)((char*)buf->slotsEmptyMutex + sizeof(sem_t));

        if (sem_init(buf->mutex, 1, 1) == -1 ||
            sem_init(buf->slotsEmptyMutex, 1, buf->size) == -1 ||
            sem_init(buf->slotsFullMutex, 1, 0) == -1) {
            perror("sem_init error");
            exit(1);
        }
    } else {
        fprintf(stderr, "Invalid type of buffer!\n");
        exit(1);
    }

    int dataID = shmget(key - 2, buf->size * sizeof(char*), IPC_CREAT | 0666);
    if (dataID == -1) {
        perror("shmget error");
        exit(1);
    }

    void * dataAddr = shmat(dataID, NULL, 0);
    if (dataAddr == (void *)-1) {
        perror("shmat error");
        exit(1);
    }

    buf->data = (char**)dataAddr;

    int elemIDs[buf->size];
    for (int i = 0; i < buf->size; i++) {
        elemIDs[i] = shmget(key + i + 1, 100, IPC_CREAT | 0666);
        if (elemIDs[i] == -1) {
            perror("shmget error");
            exit(1);
        }

        void * elemAddr = shmat(elemIDs[i], NULL, 0);
        if (elemAddr == (void *)-1) {
            perror("shmat error");
            exit(1);
        }

        buf->data[i] = (char*)elemAddr;
    }
}

// Opening an existing buffer
Buffer * openBuffer(key_t key, int shmsize) {
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
    Buffer * buf = (Buffer *)shmAddr;

    if (buf->isAsync == 0) {
        int mutexID = shmget(key - 1, sizeof(sem_t) * 3, 0666);
        if (mutexID == -1) {
            perror("shmget error");
            exit(1);
        }

        void * mutexAddr = shmat(mutexID, NULL, 0);
        if (mutexAddr == (void *)-1) {
            perror("shmat error");
            exit(1);
        }

        buf->mutex = (sem_t *)mutexAddr;
        buf->slotsEmptyMutex = (sem_t *)((char*)buf->mutex + sizeof(sem_t));
        buf->slotsFullMutex = (sem_t *)((char*)buf->slotsEmptyMutex + sizeof(sem_t));
    }

    int dataID = shmget(key - 2, buf->size * sizeof(char*), 0666);
    if (dataID == -1) {
        perror("shmget error");
        exit(1);
    }

    void * dataAddr = shmat(dataID, NULL, 0);
    if (dataAddr == (void *)-1) {
        perror("shmat error");
        exit(1);
    }

    buf->data = (char**)dataAddr;

    int elemIDs[buf->size];
    for (int i = 0; i < buf->size; i++) {
        elemIDs[i] = shmget(key + i + 1, 100, 0666);
        if (elemIDs[i] == -1) {
            perror("shmget error");
            exit(1);
        }

        void * elemAddr = shmat(elemIDs[i], NULL, 0);
        if (elemAddr == (void *)-1) {
            perror("shmat error");
            exit(1);
        }

        buf->data[i] = (char*)elemAddr;
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
  sleep(DELAY);
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
    sleep(DELAY);
    return result;
}


void ringWrite (Buffer * buffer, char * item) {
    fprintf(stderr, "STARTING RING WRITE\n");
    int res1 = sem_wait(buffer->slotsEmptyMutex);
    if (res1 == -1) {
        perror("sem_wait error");
        exit(1);
    }
    fprintf(stderr, "PASSED SEM_WAIT\n");
    int res2 = sem_wait(buffer->mutex);
    if (res2 == -1) {
        perror("sem_wait error");
        exit(1);
    }
    fprintf(stderr, "PASSED SEM_WAIT TWO\n");

    /* put value into the buffer */
    strncpy(buffer->data[buffer->in], item, 99);
    buffer->in = (buffer->in + 1) % buffer->size;

    sem_post(buffer->mutex);
    sem_post(buffer->slotsFullMutex);
    fprintf(stderr, "FINISHED RING WRITE\n");
}

char * ringRead (Buffer * buffer) {
    sem_wait(buffer->slotsFullMutex);
    sem_wait(buffer->mutex);

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