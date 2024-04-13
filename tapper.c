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
#include "bufferLib.h"

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

BufferConfig bufferInfo;

// parse command line arguments function
void parse_args(int argc, char *argv[]);

int main(int argc, char *argv[]){
    // parse command line arguments
    parse_args(argc, argv);

    keys[0] = 1234;
    keys[1] = 5678;
    // number of processes is 3: p1 observe, p2 reconstruct, p3 tapplot
    int bufSize = atoi(bufferInfo.bufferSize);
    for (int i = 0; i < num_processes - 1; i++) {
        createBuffer(keys[i], SHMSIZE, bufferInfo.isAsync, bufSize, "tapper");
    }

    // default arg number for tapplot
    char argnValue[10] = "1";

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
            args[0] = program; args[1] = "-b"; args[2] = bufferInfo.isAsync; 
            args[3] = "-s"; args[4] = bufferInfo.bufferSize; args[5] = NULL;
            } else if (i == num_processes - 1) { // Last process (tapplot)
            program = "./tapplot";
            args[0] = program; args[1] = "-b"; args[2] = bufferInfo.isAsync;
            args[3] = "-s"; args[4] = bufferInfo.bufferSize; args[5] = "-n"; args[6] = argnValue; args[7] = NULL;
            } else { // Middle process (reconstruct)
            program = "./reconstruct";
            args[0] = program; args[1] = "-b"; args[2] = bufferInfo.isAsync; 
            args[3] = "-s"; args[4] = bufferInfo.bufferSize; args[5] = NULL;
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


    // exit
    return 0;
}

void parse_args(int argc, char *argv[]){
    //if (argc != 2){
      //  printf("Please specify an input file!\n");
        //exit(1);
    //}

    // flags for buffer type and size if they are set
    bool bufferTypeSet = false, bufferSizeSet = false;
    
    // set argn value for tapplot
    char argnValue[10] = "1";

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