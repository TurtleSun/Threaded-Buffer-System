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
void removeSpaces(char *);
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
    while (1)
    {
        // If stdin is a terminal, create a prompt for user input.
        if (isTerminal)
            printf("myshell> ");

        // Get input from stdin, check if we have reached the end of stdin.
        fgets(receivedInput, 1024, stdin);
        if (feof(stdin))
            break;

        // Begin processing.
        printf("GOT INPUT: %s\n", receivedInput);
        processInput(receivedInput);
    }
    return 0;
}

void processInput(char *str)
{
    // Remove trailiing newline from input
    if (str[strlen(str) - 1] == '\n')
        str[strlen(str) - 1] = '\0';

    numCmds = 0;
    while (1)
    {
        // Split input into commands based on semicolons.
        strtok(str, ";");
        if (strcmp(str, "") == 0) // Check if there are no more commands
            break;

        inputtedCmds[numCmds] = malloc(strlen(str) + 1);
        inputtedCmds = realloc(inputtedCmds, sizeof(char *) * (numCmds + 1));
        if (inputtedCmds == NULL || inputtedCmds[numCmds] == NULL)
        {
            fprintf(stderr, "malloc error.\n");
            exit(1);
        }

        // Copy command into inputtedCmds
        strcpy(inputtedCmds[numCmds], str);

        // Increment numCmds and move to next command
        numCmds++;
        str = str + strlen(str) + 1;
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
        else if (forkRet == 0) // Child process
        {
            printf("I AM CHILD - %d\n", forkRet); // Child proc
            execvp(tokens[0], tokens);
            exit(0);
        }
        else // Parent process
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
    char *buffer = malloc(strlen(str) + 1);
    if (tokens == NULL || buffer == NULL)
    {
        fprintf(stderr, "malloc error.\n");
        exit(1);
    }

    int numTokens = 0;
    int bufferIdx = 0;
    int numChars = strlen(str);
    for (int i = 0; i < numChars; i++)
    {
        if (str[i] == ' ')
        {
            buffer[bufferIdx] = '\0';

            // (Re-)allocating memory for the new token.
            tokens[numTokens] = malloc(strlen(buffer) + 1);
            tokens = realloc(tokens, sizeof(char *) * (numTokens + 1));
            if (tokens == NULL || tokens[numTokens] == NULL)
            {
                fprintf(stderr, "malloc error.\n");
                exit(1);
            }

            // Copy the buffer into the tokens array.
            strcpy(tokens[numTokens], buffer);

            // Increment numTokens and reset the buffer.
            numTokens++;
            for (int j = 0; j < bufferIdx; j++)
                buffer[j] = '\0';
            bufferIdx = 0;
        }
        else
        {
            // Copy the character into the buffer.
            buffer[bufferIdx] = str[i];
            bufferIdx++;
        }
    }

    // Copy the last token into the tokens array.
    tokens[numTokens] = malloc(strlen(buffer) + 1);
    if (tokens[numTokens] == NULL)
    {
        fprintf(stderr, "malloc error.\n");
        exit(1);
    }
    strcpy(tokens[numTokens], buffer);
    return tokens;
}

/*void removeSpaces(char *str)
{
    int numChars = strlen(str);
    for (int i = 0; i < numChars; i++)
    {
        if (str[i] == ' ')
        {
            for (int j = i; j < numChars; j++)
            {
                str[j] = str[j + 1];
            }
            numChars--;
        }
    }
}*/