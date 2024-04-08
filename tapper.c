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


#define SHMSIZE 100000

#define num_processes 3
int shmIDs[num_processes - 1];
void * shmAddr[num_processes-1];
key_t keys[2];

// structure for child processes to know if asyn or not
typedef struct {
    int buffer_size;
    bool is_async;
} BufferInfo;

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

    // child pid
    pid_t cpid;

    // check input argc correct
    if (argc != 2){
        printf("Please specify an input file!\n");
        return 0;
    }

    // execute process
    // observe, reconstruct, tapplot
    // fork and exec
    for (int i = 0; i < num_processes; ++i) {
        cpid = fork();
        if (cpid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (cpid == 0) { // Child process
            if (i == 0) { // First process (observe)
                execlp("./observe", "./observe", NULL);
            } else if (i == num_processes - 1) { // Last process (tapplot)
                execlp("./tapplot", "./tapplot", NULL);
            } else { // Middle process (reconstruct)
                execlp("./reconstruct", "./reconstruct", NULL);
            }
            perror("exec failure");
            exit(EXIT_FAILURE);
        }
    }

    // wait for all children to finish
    for (int i = 0; i < num_processes; ++i) {
        wait(NULL);
    }

    // exit
    return 0;
}

void parse_args(int argc, char *argv[]){
    if (argc != 2){
        printf("Please specify an input file!\n");
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            // Check buffer mode
            if (strcmp(argv[i + 1], "async") == 0) {
                // Asynchronous mode
                buffer_info.is_async = true;
            } else if (strcmp(argv[i + 1], "sync") == 0) {
                // Synchronous mode
                buffer_info.is_async = false;
            } else {
                fprintf(stderr, "Invalid buffer mode: %s\n", argv[i + 1]);
                exit(EXIT_FAILURE);
            }
            i++; // Skip next argument since it's part of the current flag
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            // Check buffer size
            buffer_info.buffer_size = atoi(argv[i + 1]);
            if (buffer_info.buffer_size <= 0) {
                fprintf(stderr, "Invalid buffer size: %s\n", argv[i + 1]);
                exit(EXIT_FAILURE);
            }
            }
        } else {
            fprintf(stderr, "Invalid argument: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
    // for my sanity to know
    // Output the buffering mode and buffer size for debugging purposes
    
    printf("Buffering mode: %s\n", is_async ? "async" : "sync");
    printf("Buffer size (for sync mode): %d\n", buffer_size);
}