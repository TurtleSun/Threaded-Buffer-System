#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE 4
#define MAX_TASKS 100

// Define data structure for asynchronous buffer
typedef struct {
    int data[BUFFER_SIZE];
    int count;
    int in;
    int out;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} AsyncBuffer;

// Initialize asynchronous buffer
AsyncBuffer buffer = {
    .count = 0,
    .in = 0,
    .out = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER
};

typedef struct {
    char *command;
} TaskInfo;

// Function prototypes
void *task_function(void *arg);

// Define global variables for synchronization
pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t task_completed = PTHREAD_COND_INITIALIZER;
int num_tasks;
TaskInfo tasks[MAX_TASKS]; // Array to hold task information

int main(int argc, char *argv[]) {
    // Parse command line arguments to extract tasks, buffering type, and size
    // Example: tappet -t1 observe -t2 reconstruct -t3 tapplot -b async

    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-t") == 0){
            num_tasks = atoi(argv[++i]);
            for(int j = 0; j < num_tasks; j++){
                tasks[j].command = argv[++i]; // Store commands in the task array
            }
        }
        else if(strcmp(argv[i], "-b") == 0){
            if(strcmp(argv[++i], "async") == 0){
                buffer.count = 0;
                buffer.in = 0;
                buffer.out = 0;
                pthread_mutex_init(&buffer.mutex, NULL);
                pthread_cond_init(&buffer.not_empty, NULL);
                pthread_cond_init(&buffer.not_full, NULL);
            }
            else if(strcmp(argv[i], "sync") == 0){
                buffer.count = 0;
                buffer.in = 0;
                buffer.out = 0;
            }
        }
    }

    // Create threads for each task
    pthread_t threads[MAX_TASKS];
    for (int i = 0; i < num_tasks; ++i) {
        pthread_create(&threads[i], NULL, task_function, (void *)&tasks[i]);
    }

    // Wait for threads to finish
    for (int i = 0; i < num_tasks; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Clean up resources
    pthread_mutex_destroy(&task_mutex);
    pthread_cond_destroy(&task_completed);

    return 0;
}

// Function executed by each thread
void *task_function(void *arg) {
    TaskInfo *task_info = (TaskInfo *)arg;
    char *command = task_info->command;

    pthread_mutex_lock(&task_mutex);
    // Execute shell command using system()
    system(command);
    pthread_cond_signal(&task_completed);
    pthread_mutex_unlock(&task_mutex);

    pthread_exit(NULL);
}

// Implement more task functions as needed
