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

#define SHMSIZE 100000

#define num_processes 3
int shmIDs[num_processes - 1];
void * shmAddr[num_processes-1];
key_t keys[2];

// struct to hold buffer configuration
typedef struct {
    char isAsync[6];
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
    for (int i = 0; i < num_processes - 1; i++) {
        shmIDs[i] = shmget(keys[i], SHMSIZE, IPC_CREAT | 0666);
        shmAddr[i] = shmat(shmIDs[i], NULL, 0);
    }

    // execute process
    // observe, reconstruct, tapplot
    // fork and exec
    for (int i = 0; i < num_processes; ++i) {
        pid_t cpid = fork();
        if (cpid == -1) {
            perror("fork");
            exit(1);
        } else if (cpid == 0) { // Child process
        char *program;
            if (i == 0) { // First process (observe)
            // pass in buffer size and type for when initializing buffer in children
            program = "./observe";
            } else if (i == num_processes - 1) { // Last process (tapplot)
            program = "./tapplot";
            } else { // Middle process (reconstruct)
            program = "./reconstruct";
            }
            // arguments for buffer for children passed through execvp
            char *args[] = {program, "-b", bufferInfo.isAsync, "-s", bufferInfo.bufferSize, NULL};
            execvp(program, args);
            perror("execvp failure");
            exit(1);
        }
    }

    // wait for all children to finish
    while (wait(NULL) > 0);

    // exit
    return 0;
}

void parse_args(int argc, char *argv[]){
    if (argc != 2){
        printf("Please specify an input file!\n");
        exit(1);
    }

    // flags for buffer type and size if they are set
    bool bufferTypeSet = false, bufferSizeSet = false;

    // parse command line arguments
    for (int i = 1; i < argc; i++) {
        // check for buffer type and size
        if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            // set buffer type
            strncpy(bufferConfig.bufferType, argv[i + 1], sizeof(bufferConfig.bufferType) - 1);
            bufferTypeSet = true;
            i++; // Skip the value of "-b"
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            // set buffer size
            strncpy(bufferConfig.bufferSize, argv[i + 1], sizeof(bufferConfig.bufferSize) - 1);
            bufferSizeSet = true;
            i++; // Skip the value of "-s"
        } else {
            fprintf(stderr, "Invalid argument: %s\n", argv[i]);
            exit(1);
        }
    }

    if (!bufferTypeSet || !bufferSizeSet) {
        fprintf(stderr, "Both -b (buffer type) and -s (buffer size) must be specified.\n");
        exit(1);
    }

    // Debugging output
    printf("Buffering mode: %s\n", bufferConfig.bufferType);
    printf("Buffer size: %s\n", bufferConfig.bufferSize);
}