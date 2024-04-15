// initial implementation
// paula
// tapper program 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <stdbool.h>
#include "bufferLibSimplified.h"

#define SHMSIZE 100000

#define num_processes 3
int shmIDs[num_processes - 1];
void * shmAddr[num_processes-1];
key_t keys[2];

// struct to hold buffer configuration
typedef struct {
    char isAsync[10];
    char bufferSize[10];
} BufferConfig;

typedef struct {
    int readBuf;
    char * readBufStr;
    int writeBuf;
    char* writeBufStr;
} KeyPair;

BufferConfig bufferInfo;

// parse command line arguments function
void parse_args(int argc, char *argv[]);

int main(int argc, char *argv[]){
    // parse command line arguments
    parse_args(argc, argv);

    KeyPair keypairs[num_processes];
    for (int i = 0; i < num_processes; i++) {
        keypairs[i].readBuf = -1;
        keypairs[i].writeBuf = -1;
        keypairs[i].readBufStr = (char *)malloc(10);
        keypairs[i].writeBufStr = (char *)malloc(10);
        if (keypairs[i].readBufStr == NULL || keypairs[i].writeBufStr == NULL) {
            fprintf(stderr, "malloc failed\n");
            exit(1);
        }
    }
    keypairs[0].readBuf = 0;
    strcpy(keypairs[0].readBufStr, "0");
    keypairs[0].writeBuf = 1234;
    strcpy(keypairs[0].writeBufStr, "1234");
    // number of processes is 3: p1 observe, p2 reconstruct, p3 tapplot
    int bufSize = atoi(bufferInfo.bufferSize);
    for (int i = 0; i < num_processes - 1; i++) {
        createBuffer(keypairs[i].writeBuf, bufferInfo.isAsync, bufSize);
        keypairs[i+1].readBuf = keypairs[i].writeBuf;
        keypairs[i+1].writeBuf = keypairs[i].writeBuf + 1;
        strcpy(keypairs[i+1].readBufStr, keypairs[i].writeBufStr);
        sprintf(keypairs[i+1].writeBufStr, "%d", keypairs[i+1].writeBuf);
    }

    // default arg number for tapplot
    char argnValue[10] = "2";

    // execute process
    // observe, reconstruct, tapplot
    // fork and exec
    for (int i = 0; i < num_processes; ++i) {
        pid_t cpid = fork();
        if (cpid == -1) {
            perror("fork");
            exit(1);
        } else if (cpid == 0) { // Child process
            char *program, *args[8];
            if (i == 0) { // First process (observe)
            // pass in buffer size and type for when initializing buffer in children
            program = "./observe";
            fprintf(stderr, "OBSERVE KEYS: READ: %d, WRITE: %d\n", keypairs[i].readBuf, keypairs[i].writeBuf);
            args[0] = program; args[1] = "-R"; args[2] = keypairs[i].readBufStr; 
            args[3] = "-W"; args[4] = keypairs[i].writeBufStr; args[5] = NULL;
            } else if (i == num_processes - 1) { // Last process (tapplot)
            program = "./tapplot";
            fprintf(stderr, "TAPPLOT KEYS: READ: %d, WRITE: %d\n", keypairs[i].readBuf, keypairs[i].writeBuf);
            args[0] = program; args[1] = "-n"; args[2] = argnValue;
            args[3] = "-R"; args[4] = keypairs[i].readBufStr; 
            args[5] = "-W"; args[6] = keypairs[i].writeBufStr; args[7] = NULL;
            } else { // Middle process (reconstruct)
            program = "./reconstruct";
            fprintf(stderr, "RECON KEYS: READ: %d, WRITE: %d\n", keypairs[i].readBuf, keypairs[i].writeBuf);
            args[0] = program; args[1] = "-R"; args[2] = keypairs[i].readBufStr; 
            args[3] = "-W"; args[4] = keypairs[i].writeBufStr; args[5] = NULL;
            }
            // arguments for buffer for children passed through execvp
            execvp(program, args);
            perror("execvp failure");
            exit(1);
        }
    }

    // wait for all children to finish
    while (wait(NULL) > 0);

    // detach and remove shared memory
    for (int i = 0; i < num_processes - 1; i++) {
        shmdt(shmAddr[i]);
        shmctl(shmIDs[i], IPC_RMID, NULL);
    }

    fprintf(stderr, "I AM RETURNING\n");

    // exit
    return 0;
}

void parse_args(int argc, char *argv[]){

    // flags for buffer type and size if they are set
    bool bufferTypeSet = false, bufferSizeSet = false;
    
    // set argn value for tapplot
    char argnValue[10] = "2";

    // parse command line arguments
    for (int i = 1; i < argc; i++) {
        // check for buffer type and size
        if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            // set buffer type
            strncpy(bufferInfo.isAsync, argv[i + 1], sizeof(bufferInfo.isAsync) - 1);
            bufferTypeSet = true;
            i++; // Skip the value of "-b"
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            // set buffer size
            strncpy(bufferInfo.bufferSize, argv[i + 1], sizeof(bufferInfo.bufferSize) - 1);
            bufferSizeSet = true;
            i++; // Skip the value of "-s"
        // set argn value for tapplot
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            strncpy(argnValue, argv[i + 1], sizeof(argnValue) - 1);
            i++; // Skip the value of "-n"
        } else {
            fprintf(stderr, "Invalid argument: %s\n", argv[i]);
            exit(1);
        }
    }

    if (!bufferTypeSet || !bufferSizeSet) {
        fprintf(stderr, "Both -b (buffer type) and -s (buffer size) must be specified.\n");
        exit(1);
    }
}