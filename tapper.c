// initial implementation
// paula
// tapper program 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char *argv[]){
    // number of processes is 3: p1 observe, p2 reconstruct, p3 tapplot
    int num_processes = 3;

    // number of pipes is 2 
    // one from observe to reconstrcut, another one from reconstruct to tapplot
    int pipefd[2][2];
    // pipefd[0]: pipe 0 (between p1 and p2), pipefd[1]: pipe 1 between p2 and p3
    //pipefd[x][0]: read endpoint pipe x, pipefd[x][1]: write endpoint pipe x

    // child pid
    pid_t cpid;

    // check input argc correct
    if (argc != 2){
        printf("Please specify an input file!\n");
        return 0;
    }

    // create pipes
    for (int i = 0; i < num_processes - 1; i++){
        if (pipe(pipes[i]) == -1){
            perror("pipe");
            exit(1);
        }
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
                execute_process("observe", STDIN_FILENO, pipes[i][1]);
            } else if (i == num_processes - 1) { // Last process (tapplot)
                execute_process("tapplot", pipes[i-1][0], STDOUT_FILENO);
            } else { // Middle process (reconstruct)
                execute_process("reconstruct", pipes[i-1][0], pipes[i][1]);
            }
        } else if (cpid > 0) {
            // parent process
            // do something??
            // close the unused file descpritors 
            if (i == 0) {
                close(pipes[i][1]);
            } else if (i == num_processes - 1) {
                close(pipes[i-1][0]);
            } else {
                close(pipes[i-1][0]);
                close(pipes[i][1]);
            }
        }
    }

    // parent process closes all ends of the pipes and waits for children to finish
    // parent process is the tapper program
    for (int i = 0; i < 2; ++i) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // wait for all children to finish
    for (int i = 0; i < num_processes; ++i) {
        wait(NULL);
    }

    // exit
    return 0;
}


///// execlp or execv
void execute_process(char *program, int read_fd, int write_fd) {
    // Redirect STDIN if read_fd is not standard input
    if (read_fd != STDIN_FILENO) {
        dup2(read_fd, STDIN_FILENO);
        close(read_fd);
    }

    // Redirect STDOUT if write_fd is not standard output
    if (write_fd != STDOUT_FILENO) {
        dup2(write_fd, STDOUT_FILENO);
        close(write_fd);
    }

    execlp(program, program, (char *)NULL);
    perror("execlp"); // execlp only returns on error
    exit(EXIT_FAILURE);
}

