#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int numCmds;
char **inputtedCmds;

int numPids;
int *pids;

char *receivedInput;

void processInput(char *);
char **splitBySpace(char *);

int main(int argc, char **argv)
{
    numPids = 0;
    int isTerminal = isatty(0);

    // Initialize memory, assuming one element for now.
    receivedInput = malloc(1024); // Unknown input size, so assume fixed buffer size.
    inputtedCmds = malloc(sizeof(char *));
    pids = malloc(sizeof(int));

    // Check if malloc was successful.
    if (pids == NULL || receivedInput == NULL || inputtedCmds == NULL)
    {
        fprintf(stderr, "malloc error.\n");
        exit(1);
    }

    // Create a loop to get input from stdin and process it.
    int continueProcessing = 1;
    while (continueProcessing)
    {
        // If stdin is a terminal, create a prompt for user input.
        if (isTerminal)
            printf("myshell> ");

        // Get input from stdin, check if we have reached the end of stdin.
        fgets(receivedInput, 1024, stdin);
        if (feof(stdin))
            continueProcessing = 0;

        // Begin processing.
        printf("GOT INPUT: %s\n", receivedInput);
        processInput(receivedInput);
    }
    return 0;
}

void processInput(char *str)
{
    char * strCopy = malloc(strlen(str) + 1);
    if (strCopy == NULL)
    {
        fprintf(stderr, "malloc error.\n");
        exit(1);
    }
    strcpy(strCopy, str);

    // Remove trailiing newline from input
    if (strCopy[strlen(strCopy) - 1] == '\n')
        strCopy[strlen(strCopy) - 1] = '\0';

    numCmds = 0;
    while (1)
    {
         // Check if there are no more commands
        if (strcmp(strCopy, "") == 0)
            break;

        // Split input into commands based on semicolons.
        strtok(strCopy, ";");

        // Allocating memory for the command.
        inputtedCmds[numCmds] = malloc(strlen(strCopy) + 1);
        inputtedCmds = realloc(inputtedCmds, sizeof(char *) * (numCmds + 1));
        if (inputtedCmds == NULL || inputtedCmds[numCmds] == NULL)
        {
            fprintf(stderr, "malloc error.\n");
            exit(1);
        }

        // Copy command into inputtedCmds
        strcpy(inputtedCmds[numCmds], strCopy);

        // Increment numCmds and move to next command
        numCmds++;
        strCopy = strCopy + strlen(strCopy) + 1;
    }

    // Iterate through each set of commands identified.
    for (int i = 0; i < numCmds; i++)
    {
        // Extract the command and arguments by means of checking the spaces.
        char **tokens = splitBySpace(inputtedCmds[i]);

        // Fork and execute the command.
        int forkRet = fork();
        if (forkRet == -1) // Fork failed
        {
            fprintf(stderr, "fork error.\n");
            exit(1);
        }
        else if (forkRet == 0) // Child process - do the command.
        {
            printf("I AM CHILD - %d\n", forkRet); // Child proc
            execvp(tokens[0], tokens);
            exit(0);
        }
        else // Parent process - just wait for the child.
        {
            printf("I AM PARENT - %d\n", forkRet);
            waitpid(forkRet, NULL, 0);
        }
    }
}

// Takes a string and splits it into substrings based on spaces.
char **splitBySpace(char *str)
{
    // Initializing memory, assuming one element for now in tokens.
    char **tokens = malloc(sizeof(char *));
    if (tokens == NULL)
    {
        fprintf(stderr, "malloc error.\n");
        exit(1);
    }

    int numTokens = 0;
    while (1) {
        // Check if there are no more commands
        if (strcmp(str, "") == 0)
            break;

        // Split input into commands based on spaces.
        str = strtok(str, " ");

        // Check if the remaining string is empty. (Space gets replaced with null char in strtok)
        if (str == NULL)
            break;

        // Allocating memory    
        tokens[numTokens] = malloc(strlen(str) + 1);
        tokens = realloc(tokens, sizeof(char *) * (numTokens + 1));
        if (tokens == NULL || tokens[numTokens] == NULL)
        {
            fprintf(stderr, "malloc error.\n");
            exit(1);
        }

        // Copy the current string into the tokens array.
        strcpy(tokens[numTokens], str);
        numTokens++;
        str = str + strlen(str) + 1;
    }

    return tokens;
}