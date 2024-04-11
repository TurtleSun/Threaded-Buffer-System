#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "bufferLib.h"

#define MAX_TASK_SIZE 1024
#define MAX_NAME_LEN 100
#define MAX_VALUE_LEN 100
#define MAX_LINE_LEN 200

#define MAX_PAIRS 100

typedef struct {
    char name[MAX_NAME_LEN];
    char value[MAX_VALUE_LEN];
    int count;
} Pair;

// Define KnownValues struct
typedef struct {
    Pair pairs[MAX_PAIRS];
    int count;
    char endName[MAX_NAME_LEN];
} KnownValues;

/* // Structs for buff
typedef struct {
    char **data;
    int size;  // Size of the data array
    int count; // Number task rn
    int head;  // Head of the queue of data
    int isAsync; // int boolean for if tasks should be completed in async
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} Buffer; */

// Function prototypes
void *task_function(void *arg);
void *observe_function(void *arg);
void *reconstruct_function(void *arg);

void parseData(const char* data, Pair* outPair);
void updateLastKnownValues(Pair* newPair, KnownValues* knownValues);
void findEndName(KnownValues *values);
void compileSample(KnownValues* knownValues, char* outSample);

void *tapplot_function(void *arg);

// Define global variables for synchronization
int num_tasks;
int buff_size;
char **tasks; // Array to hold task information
char *knowntests[] = {"observe", "reconstruct", "tapplot"};
char * testFile;
Buffer buffer;

int main(int argc, char *argv[]) {
    // Parse command line arguments to extract tasks, buffering type, and size
    // Example: tappet -t1 observe -t2 reconstruct -t3 tapplot -b async <optional testFile> 
                            // and if -b sync -s <optional buffer_size>

    tasks = (char **)malloc(MAX_TASK_SIZE * sizeof(char *));

    // Initialize buffer
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0) {
            num_tasks = atoi(argv[++i]);
            tasks[num_tasks - 1] = strdup(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0) {
            if (strcmp(argv[++i], "async") == 0) {
                buff_size = 4;
                buffer = initBuffer("async", buff_size);
            } else if (strcmp(argv[i], "sync") == 0) {
                for (int j = i + 1; j < argc; j++) {
                    if (strcmp(argv[j], "-s")) {
                        buff_size = atoi(argv[j]);
                    }
                    if (j == argc - 1) {
                        buff_size = 1;
                    }
                }
                buffer = initBuffer("sync", buff_size);
            }
            if (++i == argc - 1) {
                testFile = stdin;
            }
        } else if (i == argc - 1) {
            testFile = argv[i];
        }
    }

    // Create threads for each task
    pthread_t threads[num_tasks];
    for (int i = 0; i < num_tasks; ++i) {
        char *task_name = tasks[i];
        char function_name[MAX_TASK_SIZE + 10]; // Extra space for "_function" suffix

        // Create the function name by appending "_function" to the task name
        snprintf(function_name, sizeof(function_name), "%s_function", task_name);

        // Check if the task name is contained in knowntests
        int is_known_task = 0;
        for (int j = 0; j < sizeof(knowntests) / sizeof(knowntests[0]); ++j) {
            if (strcmp(task_name, knowntests[j]) == 0) {
                is_known_task = 1;
                break;
            }
        }

        if (is_known_task) {
            // Create thread with the specific task function
            pthread_create(&threads[i], NULL, function_name, (void *)&buffer);
        } else {
            // Create thread with the generic task function
            pthread_create(&threads[i], NULL, task_function, (void *)tasks[i]);
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < num_tasks; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Free allocated memory
    for (int i = 0; i < num_tasks; ++i) {
        free(tasks[i]);
    }

    return 0;
}


// Function to initialize the buffer with a specified size
/* void init_buffer(Buffer *buffer, int size, int isAsync) {
    buffer->data = (char **)malloc(size * sizeof(char *));
    if (buffer->data == NULL) {
        // Handle memory allocation error
        exit(EXIT_FAILURE);
    }
    buffer->size = size;
    buffer->count = 0;
    buffer->head = 0;
    buffer->isAsync = isAsync;
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->not_empty, NULL);
    pthread_cond_init(&buffer->not_full, NULL);
} */

// Function executed by each thread
void *task_function(void *arg) {
    char *command = arg;

    // Execute shell command using system()
    system(command);

    pthread_exit(NULL);
}

void *observe_function(void *arg){
    Buffer *buffer = (Buffer *)arg;

    // Initialize the buffer with received type and size
    // Buffer shmBuffer = initBuffer(bufferType, bufferSize);

    // read and parse from either a file or stdin
    char line[MAX_LINE_LEN];
    char lastValue[MAX_VALUE_LEN] = {0};

    // loop to read from stdin
    while (fgets(line, MAX_LINE_LEN, stdin) != NULL){
        // parse the line
        // remove newline character
        line[strcspn(line, "\n")] = 0;
        // split the line into name and value
        char* name = strtok(line, "=");
        char* value = strtok(NULL, "");

        // if the line is malformed
        if (name == NULL || value == NULL){
            printf("Malformed input: %s\n", line);
            continue;
        }

        // compare current value with last value, if they are different write value in shared memory
        if(strcmp(lastValue, value) != 0) {
            strcpy(lastValue, value);
            // Write to shared memory
            //TODO: Change shm_addr here to an attribute (slot) of a structure that we cast shm_addr to.
            // snprintf(shm_addr, MAX_LINE_LEN, "%s=%s", name, value);

            // Prepare the string to write into the buffer
            char bufferData[MAX_LINE_LEN];
            snprintf(bufferData, sizeof(bufferData), "%s=%s", name, value);
            
            // Write into the buffer based on its type
            writeBuffer(buffer, bufferData);
        }
    }
    // once it is done writing data to the buffer, set the reading flag to 1
    buffer->reading = 1;
    // write into the buffer, at the very end, the end marker
    writeBuffer(buffer, "END_OF_DATA");

    pthread_exit(NULL);
}

void *reconstruct_function(void *arg){

    Buffer *buffer = (Buffer *)arg;

    // initialize known values struct to hold the last known values
    KnownValues knownValues;
    // char array to hold the data
    char data[MAX_LINE_LEN];
    // initialize a pair to hold the parsed data
    Pair parsedData;

    ///////////////////// READING FROM OBSERVE
    // read from shared memory between observe and reconstruct and reconstruct the data logic in separate function
    // while reading flag is 0 then the data is not ready to read
    while (!buffer->reading) {
        // sleep briefly
        usleep(1000);
    }

    // reading from the buffer 
    // process data from obsrecBuffer, data observe wrote
    while (1){
        char* dataObs = readBuffer(buffer);
        // check for END marker symbolizing no more data to read
        if (strcmp(dataObs, "END_OF_DATA") == 0) {
            break;
        }
        if (dataObs != NULL) {
            parseData(dataObs, &parsedData);
            updateLastKnownValues(&parsedData, &knownValues);
            findEndName(&knownValues);
            // if we have found the end name, compile the sample
            if (strcmp(parsedData.name, knownValues.endName) == 0) {
                char sample[MAX_TASK_SIZE];
                compileSample(&knownValues, sample);
                printf("There is the sample: %s\n", sample);  
                writeBuffer(buffer, sample);
            }
        }
    }

    // once it is done writing data to the buffer, set the reading flag to 1
    buffer->reading = 1;
    // write into the buffer, at the very end, the end marker
    writeBuffer(buffer, "END_OF_DATA");

    pthread_exit(NULL);
}

void *tapplot_function(void *arg){
    Buffer *buffer = (Buffer *)arg;

    // Open a pipe to gnuplot
    FILE *gnuplotPipe = popen("gnuplot -persistent", "w");
    if (!gnuplotPipe) {
        fprintf(stderr, "Error opening pipe to gnuplot");
        exit(EXIT_FAILURE);
    }

    // Prepare Gnuplot
    fprintf(gnuplotPipe, "set term png\n");
    fprintf(gnuplotPipe, "set output 'output.png'\n");
    fprintf(gnuplotPipe, "set title 'Data Plot'\n");
    fprintf(gnuplotPipe, "set xlabel 'Sample Number'\n");
    fprintf(gnuplotPipe, "set ylabel 'Value'\n");
    fprintf(gnuplotPipe, "plot '-' using 1:2 with lines title 'Field %d'\n", 0);


    // read from shared memory
    // ... TODO: finish this
    while (1) {
        // if reading flag is 0 then the data is not ready to read
        while (!buffer->reading) {
            // sleep briefly
            usleep(1000);
        }
        
        // reading from the buffer
        // reading from the buffer 
        while (1){
            char* data = readBuffer(buffer);
            // check for END marker symbolizing no more data to read
            if (strcmp(data, "END_OF_DATA") == 0) {
                break;
            }
            if (data != NULL) {
                processAndPlotData(data);            }
        }
    }

    pthread_exit(NULL);
}

// parseData function
// splits a "name=value" string into a Pair struct.
void parseData(const char* data, Pair* outPair) {
    // up to 99 characters for name, up to 99 characters for value, and up to 1 character for the '='
    sscanf(data, "%99[^=]=%99[^\n]", outPair->name, outPair->value);
}

// updateLastKnownValues function
// updates or adds to the list of last known values.
void updateLastKnownValues(Pair* newPair, KnownValues* knownValues) {
    int unique = 0;
    // for each known value
    for (int i = 0; i < knownValues->count; ++i) {
        // if the new name is found in the list of known values, update the value
        if (strcmp(knownValues->pairs[i].name, newPair->name) == 0) {
            strcpy(knownValues->pairs[i].value, newPair->value);
            knownValues->pairs[i].count++;
            unique = 1;
            break;
        }
    }
    // if the name is not found in the list of known values, add it
    if (!unique && knownValues->count < MAX_PAIRS) {
        Pair selectedPair = knownValues->pairs[knownValues->count];
        strcpy(selectedPair.name, newPair->name);
        strcpy(selectedPair.value, newPair->value);
        knownValues->pairs[knownValues->count].count = 1;
        knownValues->count++;
    }
}

// findEndName function
// checks if the name is the end name
void findEndName(KnownValues *values) {
    // go through the values in KnownValues
    // find the first value whose count is nonzer (repeated)
    // end value is the name before it

    if (values->count > 0) {
        strcpy(values->endName, values->pairs[values->count - 1].name);
    }
}

void compileSample(KnownValues *values, char sample[]) {
    // Initialize the sample string
    sample[0] = '\0'; // Ensure the string is empty initially

    // Iterate over the known values and concatenate them into the sample string
    for (int i = 0; i < values->count; ++i) {
        // Concatenate the name-value pair to the sample string
        strcat(sample, values->pairs[i].name);
        strcat(sample, "=");
        strcat(sample, values->pairs[i].value);

        // Add a comma if it's not the last pair
        if (i < values->count - 1) {
            strcat(sample, ", ");
        }
    }
}

void processAndPlotData(char * data){
    // Parse the data into name and value
    Pair pair;
    parseData(data, &pair);

    // Extract the relevant value for plotting based on the argument argn
    int argn = 1; // To be changed in main
    // Example: argn = atoi(argv[1]);

    // Here, we assume that argn is the index of the value to be plotted
    char *value = pair.value; // Default value to be plotted
    printf("This is data to be plotted %s\n", value);
    // You might need to update this value based on the value at index argn in the pair

    // Open a file to write the data
    FILE *dataFile = fopen("plot_data.txt", "a");
    if (dataFile == NULL) {
        fprintf(stderr, "Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    // Write the value to the file
    fprintf(dataFile, "%s\n", value);

    // Close the file
    fclose(dataFile);

    gnuplot();
}

void gnuplot() {
    // Open a pipe to Gnuplot
    FILE *gnuplotPipe = popen("gnuplot", "w");
    if (!gnuplotPipe) {
        fprintf(stderr, "Error opening pipe to Gnuplot");
        pthread_exit(NULL);
    }

    // Gnuplot script
    fprintf(gnuplotPipe, "set terminal png\n");
    fprintf(gnuplotPipe, "set output 'plot.png'\n");
    fprintf(gnuplotPipe, "set xlabel 'Sample Number'\n");
    fprintf(gnuplotPipe, "set ylabel 'Value'\n");
    fprintf(gnuplotPipe, "set title 'Plot Title'\n");
    fprintf(gnuplotPipe, "plot 'plot_data.txt' with lines\n");
    fprintf(gnuplotPipe, "exit\n");

    // Close the pipe
    pclose(gnuplotPipe);
}