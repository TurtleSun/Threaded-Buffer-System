#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

void processCommands(char *);
//void handleInterrupt();

int main (int argc, char ** argv) {
    int numPids = 0;
    int isTerminal = isatty(0);
    char * cmds = malloc(1024);
    int * pids = malloc(1024);

    if (cmds == NULL || pids == NULL) {
        fprintf(stderr, "malloc error.\n");
    }
    //signal(SIGINT, handleInterrupt);

    if (isTerminal) {
        while(1) {
            printf("myshell> ");
            char * termInput = fgets(cmds, 1024, stdin);
            if (termInput == NULL) { // Ctrl-D issued.
                printf("\n");
                break;
            }
            printf("Got input from tty: %s", cmds);
            processCommands(cmds);
        }
    } else {
        while(1) {
            char * fileInput = fgets(cmds, 1024, stdin);
            if (fileInput == NULL || feof(stdin)) { // No more commands in the file
                break;
            }
            printf("Got file input: %s", cmds);
            processCommands(cmds);
        }
    }

    return 0;
}

void processCommands(char * cmds) {
    int forkRet = fork();
    if (forkRet == -1) {
        exit(1);
    } else if (forkRet == 0) {
        printf("I AM CHILD - %d\n", forkRet);
        exit(0);
    } else {
        printf("I AM PARENT - %d\n", forkRet);    
        waitpid(forkRet, NULL, 0); 
    }
    return;
}

//void handleInterrupt() { // Used to respond to SIGINT / Ctrl-c
//    printf("handleInterrupt successful");
//}