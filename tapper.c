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

int main(int argc, char *argv[]){
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
                execlp("observe", "observe", NULL);
            } else if (i == num_processes - 1) { // Last process (tapplot)
                execlp("tapplot", "tapplot", NULL);
            } else { // Middle process (reconstruct)
                execlp("reconstruct", "reconstruct", NULL);
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