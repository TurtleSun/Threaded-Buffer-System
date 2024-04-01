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
                close(pipes[i][READ_END]); // Close the unused read end
                execute_process("observe", STDIN_FILENO, pipes[i][WRITE_END]);
            } else if (i == num_processes - 1) { // Last process (tapplot)
                close(pipes[i-1][WRITE_END]); // Close the unused write end
                execute_process("tapplot", pipes[i-1][READ_END], STDOUT_FILENO);
            } else { // Middle process (reconstruct)
                close(pipes[i-1][WRITE_END]);
                close(pipes[i][READ_END]);
                execute_process("reconstruct", pipes[i-1][READ_END], pipes[i][WRITE_END]);
            }
        }
    }

    // Wait for all children to finish

    // parent process closes all ends of the pipes and waits for children to finish
    // parent process is the tapper program
    for (int i = 0; i < 2; ++i) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

}

