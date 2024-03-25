#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

// Used to maintain list of commands.
int numCmds;
char **inputtedCmds;

// Used to maintain list of tokens in a command.
int numTokens;
char **tokens;

// Used to maintain list of child processes (especially background tasks)
int numPids;
int *pids;

// receivedInput acts as input buffer. Compared against prevInput to check for EOF.
char *receivedInput;
char *prevInput;

// Used to maintain file descriptors for files used in redirections.
struct openedFile
{
    char *filename;
    int fd;
};
struct openedFile openedFiles[128];
int currentFileIndex;


// Used for resetting file descriptors after redirections.
int savedStdout;
int savedStdin;
int savedStderr;

void processInput(char *);
void executeCommands(char **);
char **splitBySpace(char *);
int setRedirections(char **);
void resetRedirections(void);
int openFile(char *, int);
void linkFileDescriptors(int, int);
int isFileOpened(char *);

int main(int argc, char **argv)
{
    // Save the current file descriptors for stdout, stdin, and stderr.
    savedStdout = dup(1);
    savedStdin = dup(0);
    savedStderr = dup(2);

    if (savedStdout == -1 || savedStdin == -1 || savedStderr == -1)
    {
        perror("dup error");
        exit(1);
    }

    numPids = 0;
    currentFileIndex = 0;

    int isTerminal = isatty(0);

    // Initialize memory, assuming one element for now.
    receivedInput = malloc(1024); // Unknown input size, so assume fixed buffer size.
    prevInput = malloc(1024);
    inputtedCmds = malloc(sizeof(char *));
    pids = malloc(sizeof(int));

    // Check if malloc was successful.
    if (pids == NULL || receivedInput == NULL || prevInput == NULL || inputtedCmds == NULL)
    {
        perror("malloc error");
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
        if (feof(stdin) && (strcmp(receivedInput, prevInput) == 0 || isTerminal)) // Terminal entered Ctrl-D or EOF on empty line in file.
            break;
        else if (feof(stdin)) // EOF on the same line as new input in a file, do one more iteration.
            continueProcessing = 0;

        // Copy input for next iteration to check for EOF.
        strcpy(prevInput, receivedInput);

        // Begin processing.
        processInput(receivedInput);
    }
    printf("\n");
    return 0;
}

// Processing string input to determine commands to run
void processInput(char *str)
{
    // Copy input to a new string to avoid modifying the original.
    char *strCopy = malloc(strlen(str) + 1);
    if (strCopy == NULL)
    {
        perror("malloc error");
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
            perror("malloc error");
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
        tokens = splitBySpace(inputtedCmds[i]);
        executeCommands(tokens);
    }
}

// Takes a string and splits it into substrings based on spaces.
char **splitBySpace(char *str)
{
    // Initializing memory, assuming one element for now in tokens.
    tokens = malloc(sizeof(char *));
    if (tokens == NULL)
    {
        perror("malloc error");
        exit(1);
    }

    numTokens = 0;
    while (1)
    {
        // Check if there are no more commands
        if (strcmp(str, "") == 0)
            break;

        // Split input into commands based on spaces.
        str = strtok(str, " ");

        // Check if the remaining string is empty. (Space gets replaced with null char in strtok)
        if (str == NULL)
            break;

        // Allocating memory
        tokens = realloc(tokens, sizeof(char *) * (numTokens + 1));
        tokens[numTokens] = malloc(strlen(str) + 1);
        if (tokens == NULL || tokens[numTokens] == NULL)
        {
            perror("malloc error");
            exit(1);
        }

        // Copy the current string into the tokens array.
        strcpy(tokens[numTokens], str);
        numTokens++;
        str = str + strlen(str) + 1;
    }

    return tokens;
}

// Executes the commands in the tokens array.
void executeCommands(char **tokens)
{
    int setRedReturn = setRedirections(tokens);
    if (setRedReturn == -1) {
        perror("Bad Input for Redirection");
        return;
    }

    // Fork and execute the command.
    int forkRet = fork();
    if (forkRet == -1) // Fork failed
    {
        perror("fork error");
        exit(1);
    }
    else if (forkRet == 0) // Child process - do the command.
    {
        execvp(tokens[0], tokens);
        exit(0);
    }
    else // Parent process - just wait for the child.
    {
        waitpid(forkRet, NULL, 0);
    }

    resetRedirections();
}

// Iterates over the list of tokens for a command and sets any needed redirections, in and out.
int setRedirections(char **tokens)
{
    int isRedirecting = 0;
    int newfd = -2;

    for (int i = 0; i < numTokens; i++)
    {
        isRedirecting = 0;

        // Redirect stdout and stderr to the same file
        if (strcmp(tokens[i], "&>") == 0)
        {
            int fileFD = isFileOpened(tokens[i+1]);
            if (fileFD == -1) {
                newfd = openFile(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC);
                if (newfd == -1)
                    return -1;
            } else
                newfd = fileFD;

            linkFileDescriptors(1, newfd);
            linkFileDescriptors(2, newfd);
            isRedirecting = 1;
        }

        // Redirecting stdout to a file.
        if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "1>") == 0)
        {
            int fileFD = isFileOpened(tokens[i+1]);
            if (fileFD == -1) {
                newfd = openFile(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC);
                if (newfd == -1)
                    return -1;
            } else
                newfd = fileFD;

            linkFileDescriptors(1, newfd);
            isRedirecting = 1;
        }

        // Redirecting stderr to a file.
        if (strcmp(tokens[i], "2>") == 0)
        {
            int fileFD = isFileOpened(tokens[i+1]);
            if (fileFD == -1) {
                newfd = openFile(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC);
                if (newfd == -1)
                    return -1;
            } else
                newfd = fileFD;

            linkFileDescriptors(2, newfd);
            isRedirecting = 1;
        }

        // Redirecting stdin to a file.
        if (strcmp(tokens[i], "<") == 0)
        {
            int fileFD = isFileOpened(tokens[i+1]);

            if (fileFD == -1) {
                newfd = openFile(tokens[i + 1], O_RDONLY);
                if (newfd == -1)
                    return -1;
            } else
                newfd = fileFD;

            linkFileDescriptors(0, newfd);
            isRedirecting = 1;
        }

        // We did some redirection on the current token. Adjust the tokens array accordingly for later execution.
        if (isRedirecting)
        {
            // Remove the redirection operator and the target file from the tokens array.
            for (int j = i; j < numTokens - 2; j++)
                tokens[j] = tokens[j + 2];

            // After shifting, set the last two elements to NULL.
            tokens[numTokens - 2] = NULL;
            tokens[numTokens - 1] = NULL;

            // Adjust the number of tokens, and decrement i to check the current, shifted token again.
            numTokens -= 2;
            i--;
        }
    }
    return 0;
}

// Used after executing a command to reset the file descriptors to their original state.
void resetRedirections()
{
    int resetStdout = dup2(savedStdout, 1);
    int resetStdin = dup2(savedStdin, 0);
    int resetStderr = dup2(savedStderr, 2);
    if (resetStdout == -1 || resetStdin == -1 || resetStderr == -1)
    {
        perror("dup error");
        exit(1);
    }
}

// Utilizes dup2 to link file descriptors to the target file.
void linkFileDescriptors(int oldfd, int newfd)
{
    // Duplicate target file stream to stdin.
    int dupRet = dup2(newfd, oldfd);
    if (dupRet == -1)
    {
        perror("dup error");
        exit(1);
    }
}

// Opens a file with the given flags and returns the file descriptor.
int openFile(char *file, int flags)
{
    // Open the target file.
    int newfd = open(file, flags , 0644);
    if (newfd == -1)
        return -1;

    openedFiles[currentFileIndex].filename = file;
    openedFiles[currentFileIndex].fd = newfd;
    currentFileIndex++;

    return newfd;
}

// Checks if we've previously openend a file. If so, return the file descriptor.
int isFileOpened(char * filename) {
    for (int i = 0; i < 128; i++) {
        if (openedFiles[i].filename != NULL && strcmp(openedFiles[i].filename, filename) == 0) {
            return openedFiles[i].fd;
        }
    }
    return -1;
}