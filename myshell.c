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
void executeCommands(char **, int);
void forkAndExecuteProc(char **);
char **splitBySpace(char *);
int setRedirections(char **);
void resetRedirections(void);
int setPipes(char **);
void executePipes(char ***, int);
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

        // Reset any redirections from the previous command.
        resetRedirections();
    }
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
        executeCommands(tokens, 0);
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
void executeCommands(char **tokens, int fromPipe)
{
    if (!fromPipe)
    {
        int pipesFound = setPipes(tokens);
        if (pipesFound == -1)
        {
            perror("Bad Pipe Input");
            return;
        }
        else if (pipesFound == 1)
        {
            return;
        }
    }

    int setRedReturn = setRedirections(tokens);
    if (setRedReturn == -1)
    {
        perror("Bad Input for Redirection");
        return;
    }

    int forkRet = fork();
    if (forkRet == -1) // Fork failed
    {
        perror("fork error");
        exit(1);
    }
    else if (forkRet == 0) // Child process - do the command.
    {
        int execRet = execvp(tokens[0], tokens);
        if (execRet == -1)
        {
            perror("exec error");
            exit(1);
        }
        exit(0);
    }
    else // Parent process - just wait for the child.
    {
        waitpid(forkRet, NULL, 0);
    }

    return;
}

// Iterates over the list of tokens for a command and sets any needed redirections, in and out.
int setRedirections(char **tokens)
{
    int isRedirecting = 0;
    int newFileFD = -2;

    for (int i = 0; i < numTokens; i++)
    {
        isRedirecting = 0;

        // Redirect stdout and stderr to the same file
        if (strcmp(tokens[i], "&>") == 0)
        {
            int fileFD = isFileOpened(tokens[i + 1]);
            if (fileFD == -1)
            {
                newFileFD = openFile(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC);
                if (newFileFD == -1)
                    return -1;
            }
            else
                newFileFD = fileFD;

            linkFileDescriptors(newFileFD, 1);
            linkFileDescriptors(newFileFD, 2);
            isRedirecting = 1;
        }

        // Redirecting stdout to a file.
        if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "1>") == 0)
        {
            int fileFD = isFileOpened(tokens[i + 1]);
            if (fileFD == -1)
            {
                newFileFD = openFile(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC);
                if (newFileFD == -1)
                    return -1;
            }
            else
                newFileFD = fileFD;

            linkFileDescriptors(newFileFD, 1);
            isRedirecting = 1;
        }

        // Redirecting stderr to a file.
        if (strcmp(tokens[i], "2>") == 0)
        {
            int fileFD = isFileOpened(tokens[i + 1]);
            if (fileFD == -1)
            {
                newFileFD = openFile(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC);
                if (newFileFD == -1)
                    return -1;
            }
            else
                newFileFD = fileFD;

            linkFileDescriptors(newFileFD, 2);
            isRedirecting = 1;
        }

        // Redirecting stdin to a file.
        if (strcmp(tokens[i], "<") == 0)
        {
            int fileFD = isFileOpened(tokens[i + 1]);

            if (fileFD == -1)
            {
                newFileFD = openFile(tokens[i + 1], O_RDONLY);
                if (newFileFD == -1)
                    return -1;
            }
            else
                newFileFD = fileFD;

            linkFileDescriptors(newFileFD, 0);
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

// Iterates over the list of tokens for a command and sets up commands for piping as needed.
int setPipes(char **tokens)
{
    // Check for pipes in the command.
    int numPipes = 0;
    for (int i = 0; i < numTokens; i++)
    {
        // Check if a pipe is the last token in the command - this is invalid.
        if (i == numTokens - 1 && strcmp(tokens[i], "|") == 0)
        {
            perror("Bad Pipe Input");
            return -1;
        }

        // Check if a pipe is found in the current token.
        if (strcmp(tokens[i], "|") == 0)
            numPipes++;
    }

    // No pipes found in command, return to resume normal execution in executeCommands().
    if (numPipes == 0)
    {
        return 0;
    }

    int numSubCmds = 0;
    char **subCmds[numTokens];

    for (int i = 0; i < numTokens; i++)
    {
        subCmds[i] = malloc(sizeof(char **));
        if (subCmds[i] == NULL)
        {
            perror("malloc error");
            exit(1);
        }
    }

    int currSubCmdIdx = 0;
    for (int i = 0; i < numTokens; i++)
    {
        if (strcmp(tokens[i], "|") == 0)
        {
            numSubCmds++;
            currSubCmdIdx = 0;
        }
        else
        {
            subCmds[numSubCmds] = realloc(subCmds[numSubCmds], sizeof(char *) * (currSubCmdIdx + 2));
            subCmds[numSubCmds][currSubCmdIdx] = tokens[i];
            subCmds[numSubCmds][currSubCmdIdx + 1] = NULL;
            currSubCmdIdx++;
        }
    }

    // Account for the command after the last pipe.
    numSubCmds++;

    // Execute the commands with pipes.
    executePipes(subCmds, numSubCmds);

    // Reset the file descriptors to their original state.
    resetRedirections();

    return 1;
}

// Takes in previously derived subCmds and executes them in a pipeline fashion.
void executePipes(char ** subCmds[], int numSubCmds)
{
    int numPipes = numSubCmds - 1;

    int * pipes[numPipes];
    int pipefd[2];
    
    // Iterate over each required pipe and initialize it.
    for (int i = 0; i < numPipes; i++)
    {
        pipes[i] = malloc(sizeof(int) * 2);
        if (pipes[i] == NULL)
        {
            perror("malloc error");
            exit(1);
        }

        // Creating the pipe
        int pipeRet = pipe(pipefd);
        if (pipeRet == -1)
        {
            perror("pipe error");
            exit(1);
        }

        // Initializing the individual read/write ends of the pipes with FDs.
        pipes[i][0] = pipefd[0];
        pipes[i][1] = pipefd[1];
    }

    // Iterate over each command and execute them
    for (int i = 0; i < numSubCmds; i++) {
        int forkRet = fork();

        // The fork failed for whatever reason.
        if (forkRet == -1)
        {
            perror("fork error");
            exit(1);
        }

        // Fork executed - I am now the child process, so do the command.
        else if (forkRet == 0)
        {
            // All but first command will read from a pipe that we link to stdin.
            if (i != 0) {
                linkFileDescriptors(pipes[i-1][0], 0);
                close(pipes[i-1][1]);
            }

            // All but last command will write into a pipe that we link to stdout.
            if (i != numSubCmds - 1) {
                linkFileDescriptors(pipes[i][1], 1);
                close(pipes[i][0]);
            }

            // Execute the command.
            int execRet = execvp(subCmds[i][0], subCmds[i]);
            if (execRet == -1)
            {
                perror("exec error");
                exit(1);
            }

            exit(0);
        }

        // Fork executed - I am the parent, just close pipe connections and wait for child.
        else
        {
            // Closing unneeded pipe connections.
            if (i != 0) {
                close(pipes[i-1][0]);
                close(pipes[i-1][1]);
            }

            // Waiting for the child (command) to execute.
            int waitRet = waitpid(forkRet, NULL, 0);
            if (waitRet == -1)
            {
                perror("waitpid error");
                exit(1);
            }
        }
    }
    return;
}

// Utilizes dup2 to link file descriptors to the target file.
void linkFileDescriptors(int oldfd, int newfd)
{
    // Duplicate target file stream to stdin.
    int dupRet = dup2(oldfd, newfd);
    if (dupRet == -1)
    {
        perror("dup error");
        exit(1);
    }
}

// Opens a file with the given flags and returns the file descriptor.
int openFile(char *file, int flags)
{
    if (currentFileIndex == 128)
    {
        perror("Too many files opened");
        return -1;
    }

    // Open the target file.
    int newFileFD = open(file, flags, 0644);
    if (newFileFD == -1)
        return -1;

    openedFiles[currentFileIndex].filename = file;
    openedFiles[currentFileIndex].fd = newFileFD;
    currentFileIndex++;

    return newFileFD;
}

// Checks if we've previously openend a file. If so, return the file descriptor.
int isFileOpened(char *filename)
{
    for (int i = 0; i < 128; i++)
    {
        if (openedFiles[i].filename != NULL && strcmp(openedFiles[i].filename, filename) == 0)
        {
            return openedFiles[i].fd;
        }
    }
    return -1;
}