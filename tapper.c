// initial implementation
// paula
// tapper program 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
    // number of processes is 3: p1 observe, p2 reconstruct, p3 tapplot
    int num_processes = 3;
    // number of pipes is 2 
    // one from observe to reconstrcut, another one from reconstruct to tapplot
    int pipes[2][2];

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
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Child process
            if (i == 0) { // First process (observe)
                close(pipes[i][0]); // Close the unused read end
                execute_process("observe", STDIN_FILENO, pipes[i][1]);
            } else if (i == num_processes - 1) { // Last process (tapplot)
                close(pipes[i-1][1]); // Close the unused write end
                execute_process("tapplot", pipes[i-1][0], STDOUT_FILENO);
            } else { // Middle process (reconstruct)
                close(pipes[i-1][1]);
                close(pipes[i][0]);
                execute_process("reconstruct", pipes[i-1][0], pipes[i][1]);
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

