#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <setjmp.h>

// myshell.c - a really bad shell program: Jake Gustin

// Used to maintain metadata about commands.
struct command {
    char * str;
    char ** tokens;
    int isBkgd;
    int numTokens;
};

// Used to maintain metadata about a list of commands.
struct commandList {
    struct command * cmd;
    int numCmds;
};

// Used to maintain metadata about a list of processes.
struct process {
    pid_t pid;
    char * cmd;
    int isBkgd;
};

// Used to maintain metadata about a list of processes.
struct processList {
    struct process * proc;
    int numProcs;
};
struct processList procList = {NULL, 0};

// Used to maintain file descriptors for files used in redirections.
struct openedFile
{
    char *filename;
    int fd;
};

// Used to maintain metadata about a list of opened files.
struct openedFileList
{
    struct openedFile *files;
    int numFiles;
};
struct openedFileList openFileList = {NULL, 0};

// Used for resetting file descriptors after redirections.
int savedStdout;
int savedStdin;
int savedStderr;

sigjmp_buf ctrlc_buf;

// Function prototypes.
struct commandList processInput(char *);
struct command tokenizeCommand(struct command);
struct command initializeCommand();
struct process initializeProcess();
struct commandList addCommand(struct commandList, struct command);
struct processList addProcess(struct processList, struct process);
struct processList removeProcess(struct processList, int);
int openFile(struct openedFileList, char *, int);
struct openedFileList addFile(struct openedFileList, struct openedFile);
struct openedFileList resetFileList(struct openedFileList);
void executeCommands(struct command);
void processPipeCommands(struct command);
void executePipeCommands(struct commandList);
int setRedirections(struct command *);
void resetRedirections();
void clearAllForegroundProcs();
void clearAllProcs();
void informBackgroundCompletion();
//struct command processQuotes(struct command);


int main(int argc, char **argv)
{
    // Check if stdin is a terminal.
    int isTerminal = isatty(0);
    
    // Save the current file descriptors for stdout, stdin, and stderr.
    savedStdout = dup(1);
    savedStdin = dup(0);
    savedStderr = dup(2);

    if (savedStdout == -1 || savedStdin == -1 || savedStderr == -1)
    {
        perror("dup error");
        exit(1);
    }

    // Initialize memory, assuming one element for now.
    char * receivedInput = malloc(1024); // Unknown input size, so assume fixed buffer size.
    char * prevInput = malloc(1024);

    // Check if malloc was successful.
    if (receivedInput == NULL || prevInput == NULL)
    {
        perror("malloc error");
        exit(1);
    }

    signal(SIGINT, clearAllForegroundProcs);
    signal(SIGCHLD, informBackgroundCompletion);
    while (1)
    {
        // Set up a signal handler for SIGINT (Ctrl-C).
        if (sigsetjmp(ctrlc_buf, 1) == 1)
        {
            resetRedirections();
            printf("\n");
        }

        // If stdin is a terminal, create a prompt for user input.
        if (isTerminal)
            printf("myshell> ");

        // Get input from stdin, check if we have reached the end of stdin.
        fgets(receivedInput, 1024, stdin);

        // Catch the case where the user presses ctrl+d twice in a row.
        if (feof(stdin) && strncmp(receivedInput, prevInput, strlen(receivedInput)) == 0) {
            printf("\n");
            break;
        } else {
            strcpy(prevInput, receivedInput);
        }

        // Begin processing.
        struct commandList cmdList = processInput(receivedInput);
        if (cmdList.numCmds == -1) {
            continue;
        }
        for (int i = 0; i < cmdList.numCmds; i++)
            executeCommands(cmdList.cmd[i]);

        // Reset any redirections and close any applicable connections.
        resetRedirections();
        openFileList = resetFileList(openFileList);

        // Check if we have reached the end of stdin.
        if (feof(stdin)) {
            break;
        }
    }
    return 0;
}

struct commandList processInput(char * str) {
    // Remove newline character from input.
    if (str[strlen(str) - 1] == '\n')
        str[strlen(str) - 1] = '\0';

    // Replace &> operator with special character to avoid strtok issues.
    char * tempStr = malloc(1024);
    char * tempStrPtr = tempStr;
    if (tempStr == NULL)
    {
        perror("malloc error");
        exit(1);
    }
    strcpy(tempStr, str);

    while ((tempStr = strstr(tempStr, "&>")) != NULL) {
        // replace &> with special string that doesn't have &
        memcpy(tempStr, "8>", 2);
        tempStr += 2;
    }

    // Set our current string to the result of the replacement.
    strcpy(str, tempStrPtr);

    // Initialize memory, same size as full buffer for now.
    char * foregroundStr = malloc(1024);
    char * backgroundStr = malloc(1024);
    if (foregroundStr == NULL || backgroundStr == NULL)
    {
        perror("malloc error");
        exit(1);
    }

    // Initialize command list.
    struct commandList cmdList = {NULL, 0};

    // Begin processing.
    while (1) {

        // Check if we have reached the end of the input.
        if (strcmp(str, "") == 0)
            break;

        // Initialize a new command.
        struct command cmd = initializeCommand();

        // Check if the next command(s) is/are a background task.
        strcpy(foregroundStr, str);
        strcpy(backgroundStr, str);

        strtok(foregroundStr, ";");
        strtok(backgroundStr, "&");

        if (strlen(backgroundStr) < strlen(foregroundStr)) {
            cmd.isBkgd = 1;
            strcpy(cmd.str, backgroundStr);
        } else {
            cmd.isBkgd = 0;
            strcpy(cmd.str, foregroundStr);
        }

        // Update cmd's tokenized command attribute.
        cmd = tokenizeCommand(cmd);
        //cmd = processQuotes(cmd);
        if (cmd.numTokens == -1) {
            return (struct commandList){NULL, -1};
        }

        // Add current command to the command list.
        cmdList = addCommand(cmdList, cmd);

        // Onto the next command.
        str = str + strlen(cmd.str) + 1;
    }
    return cmdList;
}

void executeCommands(struct command cmd) {
    // Empty command!
    if (cmd.numTokens == 0)
        return;

    // Pipe commands require further processing.
    if (strchr(cmd.str, '|') != NULL)
    {
        processPipeCommands(cmd);
        return;
    }

    int redirRet = setRedirections(&cmd);
    if (redirRet == -1) {
        perror("Invalid Redirection(s)");
        return;
    }

    int forkRet = fork();
    if (forkRet == -1)
    {
        perror("fork error");
        return;
    } else if (forkRet == 0) {
        // Child process.
        setpgid(0, 0);
        int execRet = execvp(cmd.tokens[0], cmd.tokens);
        if (execRet == -1) {
            perror("execvp error");
            exit(1);
        }
        exit(0);    
    } else {
        // Parent process.
        procList = addProcess(procList, (struct process){forkRet, cmd.str, cmd.isBkgd});
        if (!(cmd.isBkgd)) {
            waitpid(forkRet, NULL, 0);
            procList = removeProcess(procList, forkRet);
        }
    }
    return;
}

void processPipeCommands(struct command cmd) {
    // Initialize memory, assuming one element for now.
    struct commandList subCmdList = {NULL, 0};

    // Split the command string into individual commands.
    char * str = malloc(strlen(cmd.str) + 1);
    if (str == NULL)
    {
        perror("malloc error");
        exit(1);
    }
    strcpy(str, cmd.str);

    while (1) {
        // Check if there are no more commands.
        if (strcmp(str, "") == 0)
            break;

        // Initialize a new command.
        struct command newCmd = initializeCommand();
        newCmd.isBkgd = cmd.isBkgd;

        // Split input into commands based on pipes.
        str = strtok(str, "|");

        // Check if the remaining string is empty. (Pipe gets replaced with null char in strtok)
        if (str == NULL)
            break;

        // Copy the current string into the new command.
        strcpy(newCmd.str, str);

        // Update newCmd's tokenized command attribute.
        newCmd = tokenizeCommand(newCmd);

        // Add current command to the command list.
        subCmdList = addCommand(subCmdList, newCmd);

        // Onto the next command.
        str = str + strlen(newCmd.str) + 1;
    }
    executePipeCommands(subCmdList);
    resetRedirections();
    return;
}

void executePipeCommands(struct commandList cmdList) {
    int numPipes = cmdList.numCmds - 1;

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

    int forkRets[cmdList.numCmds];

    // Iterate over each command and execute them
    for (int i = 0; i < cmdList.numCmds; i++) {

        int forkRet = fork();
        forkRets[i] = forkRet;

        // The fork failed for whatever reason.
        if (forkRet == -1)
        {
            perror("fork error");
            exit(1);
        }

        // Fork executed - I am now the child process, so do the command.
        else if (forkRet == 0)
        {
            setpgid(0, 0);

            // All but first command will read from a pipe that we link to stdin.
            if (i != 0) {
                dup2(pipes[i-1][0], 0);
                close(pipes[i-1][1]);
            }

            // All but last command will write into a pipe that we link to stdout.
            if (i != cmdList.numCmds - 1) {
                dup2(pipes[i][1], 1);
                close(pipes[i][0]);
            }

            setRedirections(&(cmdList.cmd[i]));

            // Execute the command.
            int execRet = execvp(cmdList.cmd[i].tokens[0], cmdList.cmd[i].tokens);
            if (execRet == -1)
            {
                perror("exec error");
                exit(1);
            }
            exit(0);
        }

        // Fork executed - I am the parent, just close pipe connections and wait for child.
        else {
            if (i != 0) {
                close(pipes[i-1][0]);
                close(pipes[i-1][1]);
            }
        }
    }

    // Waiting for the children (subcommands) to execute.
    for (int i = 0; i < cmdList.numCmds; i++) {
        procList = addProcess(procList, (struct process){forkRets[i], cmdList.cmd[i].str, cmdList.cmd[i].isBkgd});
        if (!(cmdList.cmd[i].isBkgd)) {
            waitpid(forkRets[i], NULL, 0);
            procList = removeProcess(procList, forkRets[i]);
        }
    }
    return;
}

int setRedirections(struct command * cmdPtr) {
    struct command cmd = *cmdPtr;

    for (int i = 0; i < cmd.numTokens - 1; i++) {
        int isRedirecting = 0;
        if (strcmp(cmd.tokens[i], ">") == 0 || strcmp(cmd.tokens[i], "1>") == 0) {
            isRedirecting = 1;
            int newFileFD = openFile(openFileList, cmd.tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC);
            if (newFileFD == -1)
                return -1;
            dup2(newFileFD, 1);
        } else if (strcmp(cmd.tokens[i], "<") == 0) {
            isRedirecting = 1;
            int newFileFD = openFile(openFileList, cmd.tokens[i + 1], O_RDONLY);
            if (newFileFD == -1)
                return -1;
            dup2(newFileFD, 0);
        } else if (strcmp(cmd.tokens[i], "2>") == 0) {
            isRedirecting = 1;
            int newFileFD = openFile(openFileList, cmd.tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC);
            if (newFileFD == -1)
                return -1;
            dup2(newFileFD, 2);
        } else if (strcmp(cmd.tokens[i], "8>") == 0) {
            isRedirecting = 1;
            int newFileFD = openFile(openFileList, cmd.tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC);
            if (newFileFD == -1)
                return -1;
            dup2(newFileFD, 1);
            dup2(newFileFD, 2);
        }

        if (isRedirecting) {
            for (int j = i; j < cmd.numTokens - 2; j++) {
                cmd.tokens[j] = cmd.tokens[j + 2];
            }

            // Set last two elements to NULL.
            cmd.tokens[cmd.numTokens - 1] = NULL;
            cmd.tokens[cmd.numTokens - 2] = NULL;

            cmd.numTokens -= 2;
            i--;
        }
    }
    *cmdPtr = cmd;
    return 0;
}

void resetRedirections() {
    dup2(savedStdout, 1);
    dup2(savedStdin, 0);
    dup2(savedStderr, 2);
}

int openFile(struct openedFileList fileList, char *filename, int flags) {
    int newFileFD = open(filename, flags, 0644);
    if (newFileFD == -1)
    {
        perror("open error");
        return -1;
    }

    addFile(fileList, (struct openedFile){filename, newFileFD});
    return newFileFD;
} 

// Tokenize a command string into an array of tokens.
struct command tokenizeCommand(struct command cmd) {
    // Initializing memory, assuming one element for now in tokens.
    char ** tokens = malloc(sizeof(char *));
    char * str = malloc(strlen(cmd.str) + 1);
    if (tokens == NULL || str == NULL)
    {
        perror("malloc error");
        exit(1);
    }

    strcpy(str, cmd.str);

    int numTokens = 0;
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
        tokens = realloc(tokens, sizeof(char *) * (numTokens + 2)); // +2 for new token and then NULL
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

    // Ensuring the last token is NULL
    tokens[numTokens] = NULL;

    // Update the command struct with the tokenized command.
    cmd.tokens = tokens;
    cmd.numTokens = numTokens;

    return cmd;
}

struct command initializeCommand() {
    struct command cmd;
    cmd.isBkgd = 0;
    cmd.numTokens = 0;
    cmd.str = malloc(1024);
    cmd.tokens = malloc(sizeof(char *));
    if (cmd.tokens == NULL || cmd.str == NULL)
    {
        perror("malloc error");
        exit(1);
    }

    return cmd;
}

struct commandList addCommand(struct commandList cmdList, struct command cmd) {
    cmdList.numCmds++;
    cmdList.cmd = realloc(cmdList.cmd, cmdList.numCmds * sizeof(struct command));
    if (cmdList.cmd == NULL && cmdList.numCmds > 0)
    {
        perror("realloc error");
        exit(1);
    }
    cmdList.cmd[cmdList.numCmds - 1] = cmd;
    return cmdList;
}

struct process initializeProcess() {
    struct process proc;
    proc.cmd = malloc(1024);
    if (proc.cmd == NULL)
    {
        perror("malloc error");
        exit(1);
    }
    proc.isBkgd = 0;
    proc.pid = 0;
    return proc;
}

struct processList addProcess(struct processList procList, struct process proc) {
    procList.numProcs++;
    procList.proc = realloc(procList.proc, procList.numProcs * sizeof(struct process));
    if (procList.proc == NULL && procList.numProcs > 0)
    {
        perror("realloc error");
        exit(1);
    }
    procList.proc[procList.numProcs - 1] = proc;
    return procList;
}

struct processList removeProcess(struct processList procList, int pid) {
    for (int i = 0; i < procList.numProcs; i++)
    {
        if (procList.proc[i].pid == pid)
        {
            for (int j = i; j < procList.numProcs - 1; j++)
            {
                procList.proc[j] = procList.proc[j + 1];
            }
            procList.numProcs--;
            procList.proc = realloc(procList.proc, procList.numProcs * sizeof(struct process));
            if (procList.proc == NULL && procList.numProcs > 0)
            {
                perror("realloc error");
                exit(1);
            }
            break;
        }
    }
    return procList;
}

struct openedFileList addFile(struct openedFileList fileList, struct openedFile file) {
    fileList.numFiles++;
    fileList.files = realloc(fileList.files, fileList.numFiles * sizeof(struct openedFile));
    if (fileList.files == NULL && fileList.numFiles > 0)
    {
        perror("realloc error");
        exit(1);
    }
    fileList.files[fileList.numFiles - 1] = file;
    return fileList;
}

struct openedFileList resetFileList (struct openedFileList fileList) {
    for (int i = 0; i < fileList.numFiles; i++)
    {
        close(fileList.files[i].fd);
    }
    fileList.numFiles = 0;
    fileList.files = realloc(fileList.files, fileList.numFiles * sizeof(struct openedFile));
    if (fileList.files == NULL && fileList.numFiles > 0)
    {
        perror("realloc error");
        exit(1);
    }
    return fileList;
}

void clearAllForegroundProcs() {
    signal(SIGINT, clearAllForegroundProcs); // Re-register signal handler
    for (int i = 0; i < procList.numProcs; i++) {
        if (!(procList.proc[i].isBkgd)) {
            kill(procList.proc[i].pid, SIGTERM);
            int waitRet = waitpid(procList.proc[i].pid, NULL, 0);
            if (waitRet == -1)
            {
                perror("waitpid error");
                exit(1);
            }
            procList = removeProcess(procList, procList.proc[i].pid);
        }
    }

    siglongjmp(ctrlc_buf, 1);
}

void clearAllProcs() {
    for (int i = 0; i < procList.numProcs; i++) {
        kill(procList.proc[i].pid, SIGTERM);
        int waitRet = waitpid(procList.proc[i].pid, NULL, 0);
        if (waitRet == -1)
        {
            perror("waitpid error");
            exit(1);
        }
        procList = removeProcess(procList, procList.proc[i].pid);
    }
}

void informBackgroundCompletion() {
    signal(SIGCHLD, informBackgroundCompletion); // Re-register signal handler
    int status;
    pid_t pid;

    // Handle background processes that called this handler.
    for (int i = procList.numProcs-1; i >= 0; i--) {
        if (procList.proc->isBkgd == 1 && waitpid(procList.proc[i].pid, &status, WNOHANG) > 0) {
            printf("Background process %s with ID %d has completed.\n", procList.proc[i].cmd, procList.proc[i].pid);
            procList = removeProcess(procList, procList.proc[i].pid);
        }
    }

    return;
}

// Merge tokens that are surrounded by quotes.
/*struct command processQuotes(struct command cmd) {

    int numTokens = cmd.numTokens;
    int numQuotes = 0;
    int strIdx = 0;
    char ** tokens = malloc(sizeof(char **) * cmd.numTokens);
    char * buffer = malloc(1024);
    if (tokens == NULL || buffer == NULL)
    {
        perror("malloc error");
        exit(1);
    }

    for (int i = 0; i < cmd.numTokens; i++) {
        tokens[i] = malloc(sizeof(char *) * strlen(cmd.tokens[i]));
        if (tokens[i] == NULL) {
            perror("malloc error");
            exit(1);
        }
    }

    // Copy tokens to temporary tokens array.
    for (int i = 0; i < cmd.numTokens; i++) {
        strcpy(tokens[i], cmd.tokens[i]);
    }

    for (int tokenInd = 0; tokenInd < numTokens; tokenInd++) {
        while(strIdx < strlen(tokens[tokenInd])) {
            if (tokens[tokenInd][strIdx] == '"' && (strIdx == 0 || tokens[tokenInd][strIdx-1] != '\\')) {
                for (int j = strIdx; j < strlen(tokens[tokenInd]); j++) {
                    tokens[tokenInd][j] = tokens[tokenInd][j+1];
                }
                strIdx--;
                numQuotes++;
            } else if (tokens[tokenInd][strIdx] == '"' && tokens[tokenInd][strIdx-1] == '\\') {
                for (int j = strIdx-1; j < strlen(tokens[tokenInd]); j++) {
                    tokens[tokenInd][j] = tokens[tokenInd][j+1];
                }
                strIdx--;
            }
            strIdx++;
        }

        if (numQuotes % 2 == 1 && tokenInd == numTokens-1 && (!isatty(1) || feof(stdin))) {
            fprintf(stderr, "Unclosed set of quotation marks!\n");
            return (struct command){NULL, NULL, 0, -1};
        } else if (numQuotes % 2 == 1 && tokenInd == numTokens-1 && isatty(1)) {
            char * buffer = malloc(1024);
            if (buffer == NULL) {
                perror("malloc error");
                exit(1);
            }
            printf("> ");
            fgets(buffer, 1024, stdin);
            strcat(tokens[tokenInd], buffer);
            tokenInd--;
        } else if (numQuotes % 2 == 1) {
            strcat(tokens[tokenInd], " ");
            strcat(tokens[tokenInd], tokens[tokenInd+1]);
            for (int j = tokenInd+1; j < numTokens-1; j++) {
                tokens[j] = tokens[j+1];
            }
            tokens[numTokens-1] = NULL;
            numTokens--;
            tokens=realloc(tokens, sizeof(char *) * numTokens);
            if (tokens == NULL) {
                perror("malloc error");
                exit(1);
            }
            tokenInd--;
        } else {
            strIdx = 0;
        }
    }
    tokens[numTokens] = NULL;
    cmd.tokens=tokens;
    cmd.numTokens=numTokens;
    return cmd;
}*/