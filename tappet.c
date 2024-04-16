#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "bufferLib_thread.h"
#include "observe_thread.h"
#include "reconstruct_thread.h"
#include "tapplot_thread.h"

#define MAX_TASK_SIZE 1024
#define MAX_NAME_LEN 50
#define MAX_VALUE_LEN 50

// Function prototypes
void * task_function(void *arg);

int main(int argc, char *argv[]) {
    // Initialize buffer
    int argn = 1; // default for tapplot to print
    int num_tasks = 0;
    int buff_size;
    char *tasks[MAX_TASK_SIZE]; // Array to hold task information
    char * testFile = "standardOut";
    char * theSync;
    Parcel *parcel = malloc(sizeof(Parcel));
    Buffer *buf1 = malloc(sizeof(Buffer));
    Buffer *buf2 = malloc(sizeof(Buffer));

    for (int i = 1; i < argc; i++){
        int j = i + 1;

        if (strcmp(argv[i], "-t1") == 0 || strcmp(argv[i], "-t2") == 0 || strcmp(argv[i], "-t3") == 0){
            num_tasks = num_tasks + 1;
            tasks[num_tasks - 1] = argv[i+1];

        } else if (strcmp(argv[i], "-b") == 0) {
            if (strcmp(argv[j], "async") == 0) {

                theSync = "async";
                buff_size = 4;

            } else if (strcmp(argv[i+1], "sync") == 0) {
                
                theSync = "sync";
                if (strcmp(argv[j+1], "-s") == 0) {
                    buff_size = atoi(argv[j+2]);
                } else {
                    buff_size = 1;
                }
            }
        } else {
            if (strcmp(argv[i-2], "-t1") == 0 || strcmp(argv[i-2], "-t2") == 0 || strcmp(argv[i-2], "-t3") == 0) {
                // its an optional argn
                argn = atoi(argv[i]);
            }
        }
    }

    initBuffer(theSync, buff_size, argn, testFile, buf1);
    initBuffer(theSync, buff_size, argn, testFile, buf2);
    initParcel(buf1, buf2, parcel);

    // Create threads for each task
    pthread_t threads[3];
    for (int i = 0; i < 3; i++) {
        char *task_name = tasks[i];

        if (strcmp(task_name, "observe") == 0){
            pthread_create(&threads[i], NULL, observe_function, buf1);
        } else if (strcmp(task_name, "reconstruct") == 0){
            pthread_create(&threads[i], NULL, reconstruct_function, parcel);
        } else if (strcmp(task_name, "tapplot") == 0){
            pthread_create(&threads[i], NULL, tapplot_function, buf2);
        } else {
            printf("Invalid task name: %s\n", task_name);
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < num_tasks; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}


// Function executed by each thread
void *task_function(void *arg) {
    char *command = arg;
    // Execute shell command using system()
    system(command);

    pthread_exit(NULL);
}