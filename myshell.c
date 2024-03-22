#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

void processCommands(char *);
// void handleInterrupt();

int main(int argc, char **argv)
{
    int numPids = 0;
    int isTerminal = isatty(0);
    char *cmds = malloc(1024);
    char *receivedInput = malloc(1024);
    int *pids = malloc(1024);

    if (cmds == NULL || pids == NULL || receivedInput == NULL)
    {
        fprintf(stderr, "malloc error.\n");
        exit(1);
    }

    while (1)
    {
        if (isTerminal)
            printf("myshell> ");

        fgets(receivedInput, 1024, stdin);
        if (feof(stdin))
            break; // Reached end of stdin, useful for file inputs
        
        printf("GOT INPUT: %s\n", receivedInput);
        processCommands(cmds);
    }
    return 0;
}

void processCommands(char *cmds)
{
    int forkRet = fork();
    if (forkRet == -1)
    { // Check if fork failed.
        exit(1);
    }
    else if (forkRet == 0)
    {
        printf("I AM CHILD - %d\n", forkRet); // Child proc
        char *args[] = {NULL};                // Basic implementation of ls for testing
        execvp("ls", args);
        exit(0);
    }
    else
    { // Parent proc.
        printf("I AM PARENT - %d\n", forkRet);
        waitpid(forkRet, NULL, 0);
    }
}

// void handleInterrupt() { // Used to respond to SIGINT / Ctrl-c
//     printf("handleInterrupt successful");
// }