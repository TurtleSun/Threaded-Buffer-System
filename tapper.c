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
#define MAX_PROCS 10

// struct to hold buffer configuration
typedef struct {
    char isAsync[10];
    char bufferSize[10];
    char argnValue[10];
    char procs[MAX_PROCS][MAX_STR_LEN];
    int numProcs;
} BufferConfig;

typedef struct {
    int readKey;
    int writeKey;
    char readKeyStr[10];
    char writeKeyStr[10];
} KeyPairs;

BufferConfig bufferInfo;
KeyPairs keyPairs[MAX_PROCS];

// parse command line arguments function
void parse_args(int argc, char *argv[]);

int main(int argc, char *argv[]){
    // parse command line arguments
    parse_args(argc, argv);

    keyPairs[0].readKey = -1;
    keyPairs[0].writeKey = 1234;
    strcpy(keyPairs[0].readKeyStr, "-1");
    strcpy(keyPairs[0].writeKeyStr, "1234");
    // number of processes is 3: p1 observe, p2 reconstruct, p3 tapplot
    int bufSize = atoi(bufferInfo.bufferSize);
    for (int i = 0; i < bufferInfo.numProcs - 1; i++) {
        createBuffer(keyPairs[i].writeKey, bufferInfo.isAsync, bufSize);
        keyPairs[i+1].readKey = keyPairs[i].writeKey;
        keyPairs[i+1].writeKey = keyPairs[i].writeKey + 1;
        strcpy(keyPairs[i+1].readKeyStr, keyPairs[i].writeKeyStr);
        sprintf(keyPairs[i+1].writeKeyStr, "%d", keyPairs[i].writeKey + 1);
    }

    // execute process
    // observe, reconstruct, tapplot
    // fork and exec
    for (int i = 0; i < bufferInfo.numProcs; ++i) {
        pid_t cpid = fork();
        if (cpid == -1) {
            perror("fork");
            exit(1);
        } else if (cpid == 0) { // Child process
            char *program, *args[8];
            program = bufferInfo.procs[i];
            args[0] = bufferInfo.procs[i];  args[1] = "-n"; args[2] = bufferInfo.argnValue; args[3] = "-R"; 
            args[4] = keyPairs[i].readKeyStr; args[5] = "-W"; args[6] = keyPairs[i].writeKeyStr; args[7] = NULL;

            // arguments for buffer for children passed through execvp
            execvp(program, args);
            perror("execvp failure");
            exit(1);
        }
    }

    // wait for all children to finish
    while (wait(NULL) > 0);

    // detach and remove shared memory
    for (int i = 0; i < bufferInfo.numProcs - 1; i++) {
        int shmID = shmget(keyPairs[i].writeKey, sizeof(Buffer), 0666);
        if (shmID == -1) {
            perror("shmget error");
            exit(1);
        }

        void * shmAddr = shmat(shmID, NULL, 0);
        if (shmAddr == (void*)-1) {
            perror("shmat error");
            exit(1);
        }
        shmdt(shmAddr);
        shmctl(shmID, IPC_RMID, NULL);
    }

    // exit
    return 0;
}

void parse_args(int argc, char *argv[]){

    // flags for buffer type and size if they are set
    bool bufferTypeSet = false, bufferSizeSet = false;
    bufferInfo.numProcs = 0;
    
    // set argn value for tapplot
    char argnValue[10] = "1";

    // parse command line arguments
    for (int i = 1; i < argc; i++) {
        // check for buffer type and size
        if (strncmp(argv[i], "-p", 2) == 0) {
            // Set process name
            int procNum = atoi(argv[i] + 2)-1;
            if (procNum < 0 || procNum > MAX_PROCS-1) {
                fprintf(stderr, "Invalid process number: %d\n", procNum);
                exit(1);
            }
            if (strcmp(argv[i+1], "observe") == 0 || strcmp(argv[i+1], "reconstruct") == 0 || strcmp(argv[i+1], "tapplot") == 0) {
                char * modifiedProcName = malloc(100);
                strcpy(modifiedProcName, "./");
                strcat(modifiedProcName, argv[i+1]);
                strcpy(bufferInfo.procs[procNum], modifiedProcName);
            } else {
                strcpy(bufferInfo.procs[procNum], argv[i+1]);
            }
            if (strncmp(argv[i+2], "-b", 2) != 0 && strncmp(argv[i+2], "-p", 2) != 0) {
                // last process has a value.
                strcpy(bufferInfo.argnValue, argv[i+2]);
                i++;
            }
            bufferInfo.numProcs++;
            i++;
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
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

    if (!bufferTypeSet || ((strcmp(bufferInfo.isAsync, "sync") == 0) && !bufferSizeSet)) {
        fprintf(stderr, "Both -b (buffer type) must be specified. If async, must specify -s (buffer size)\n");
        exit(1);
    }
}