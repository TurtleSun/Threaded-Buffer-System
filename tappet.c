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
void * task_function(void *arg);

// Define global variables for synchronization
int num_tasks = 0;
int buff_size;
char *tasks[MAX_TASK_SIZE]; // Array to hold task information
char * testFile;
Parcel parcel;

int main(int argc, char *argv[]) {
    // Parse command line arguments to extract tasks, buffering type, and size
    // Example: tappet -t1 observe -t2 reconstruct -t3 tapplot <optional arg3> -b async <optional testFile> 
                            // and if -b sync -s <optional buffer_size>

    // Currently Used Example:
        // ./tappet -t1 observe -t2 reconstruct -t3 tapplot -b async small-test-file

    // Initialize buffer
    printf("Just got in main. \n");
    printf("number of arguments: %d\n", argc);
    printf("argv 1: %s\n", argv[1]);
    printf("\n");

    int argn = 1; // default for tapplot to print

    for (int i = 1; i < argc-1; i++) {
        printf("\n");
        printf("inside the for loop! \n");
        printf("argv %d: %s\n", i+1, argv[i+1]);
        printf("\n");

        if (strstr(argv[i], "-t") != NULL) {

            num_tasks = num_tasks + 1;

            tasks[num_tasks - 1] = argv[i+1];

            printf("\n");
            printf("TASK: We've saved %s as tasks: %s\n", argv[i+1], tasks[num_tasks - 1]);
            printf("\n");

        } else if (strstr(argv[i], "-b") != NULL) {

            if (!(strstr(argv[i-2], "-t") != NULL)){ // Check if arg3 happens

                argn = atoi(argv[i-1]);
                
                printf("\n");
                printf("ARGN: We've saved %s as plotting col\n", argv[i-1]);
                printf("\n");

            }

            if (strstr(argv[i+1], "async") != NULL) {

                buff_size = 4;

                if (i+1 != argc-1){ // async isn't last arg, must be optional test-file
                    testFile = argv[i+2];
                } else {
                    testFile = "1";
                }

                printf("About to initBuff\n");

                parcel = initBuffer("async", buff_size, argn, testFile);

                printf("Came back from initBuff\n");

                printf("\n");
                printf("hi?\n");

                // THIS IS WHERE THE SEG FAULT HAPPENS
                printf("ASYNC: We've initBuffer as %s\n", argv[i+1]);
                printf("This is my saved testFile: %s\n", testFile);
                printf("\n");
                break;

            } else if (strstr(argv[i+1], "sync") != NULL) {
                
                int j = i + 1;

                if (j == argc -1) { // sync is the last argument
                    buff_size = 1;
                    testFile = "1";
                } else if (strstr(argv[j], "-s")) { // there is optional size arg
                    buff_size = atoi(argv[j+1]);
                    if (j+1 != argc-1){ // size is not last arg
                        // must be test-file
                        testFile = argv[j+2];
                    } else {
                        testFile = "1";
                    }
                } else { // no optinal size then must be optional test-file
                    testFile = argv[j+1];
                }
                
                parcel = initBuffer("sync", buff_size, argn, testFile);

                printf("\n");
                printf("SYNC: We've initBuffer as %s\n", argv[i+1]);
                printf("This is my saved testFile: %s\n", testFile);
                printf("\n");
                break;
            }
        }
    }

    printf("Just finished parsing args...\n");

    // Create threads for each task
    pthread_t threads[num_tasks];
    for (int i = 0; i < num_tasks; i++) {
        char *task_name = tasks[i];

        if (strcmp(task_name, "observe") == 0){
            pthread_create(&threads[i], NULL, observe_function, (void *)&parcel);
        } else if (strcmp(task_name, "reconstruct") == 0){
            pthread_create(&threads[i], NULL, reconstruct_function, (void *)&parcel);
        } else if (strcmp(task_name, "tapplot") == 0){
            pthread_create(&threads[i], NULL, tapplot_function, (void *)&parcel);
        } else {
            printf("SOMETHING WRONG HAPPENED\n");
            pthread_create(&threads[i], NULL, task_function, (void *)&parcel);
        }

    }

    // Wait for threads to finish
    for (int i = 0; i < num_tasks; i++) {
        pthread_join(threads[i], NULL);
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

/* void *observe_function(void *arg){
    printf("Observe thread made!\n");

    Parcel *arguemnts = (Parcel *)arg;
    Buffer buffer = arguemnts->buffer;
    int hasTesFile = arguemnts->hasTestFile;

    //printf("Observe BUFF: %p\n", &buffer);
    //printf("Observe BUFF ISASYNC: %d\n", buffer.isAsync);

    FILE *fp;
    if(hasTesFile) {
        fp = fopen(testFile, "r");
    }else {
        fp = stdin;
    }

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

        // print line
        printf("name: %s, value: %s\n", name, value);

        // compare current value with last value, if they are different write value in shared memory
        if(strcmp(getLastKnown(pairlist, name), value) != 0) {
            printf("Value changed: %s\n", value);
            strcpy(lastValue, value);
            pairlist = updateLastKnown(pairlist, name, value);
            // Write to shared memory
            //TODO: Change shm_addr here to an attribute (slot) of a structure that we cast shm_addr to.
            // snprintf(shm_addr, MAX_LINE_LEN, "%s=%s", name, value);

            // Prepare the string to write into the buffer
            char bufferData[MAX_LINE_LEN];
            snprintf(bufferData, sizeof(bufferData), "%s=%s", name, value);
            
            printf("bufferData: %s\n", bufferData);
            // Write into the buffer based on its type
            writeBuffer(&buffer, bufferData);
            // print whats inside
            printf("WRITTEN IN BUFF IS: %s\n", buffer.data[0]);
        }
    }

    // once it is done writing data to the buffer, set the reading flag to 1
    buffer.reading = 1;
    // write into the buffer, at the very end, the end marker
    writeBuffer(&buffer, "END_OF_DATA");

    if(hasTesFile){
        fclose(fp);
    }

    pthread_exit(NULL);
} */

/* void *reconstruct_function(void *arg){
    printf("Reconstruct thread made!");
    Parcel *arguemnts = (Parcel *)arg;
    Buffer buffer = arguemnts->buffer;

    //printf("Reconstruct BUFF: %p\n", &buffer);
    //printf("Reconstruct BUFF ISASYNC: %d\n", buffer.isAsync);

    // initialize known values struct to hold the last known values
    KnownValues knownValues;
    // char array to hold the data
    char data[MAX_LINE_LEN];
    // initialize a pair to hold the parsed data
    Pair parsedData;

    // reading from the buffer 
    // process data from obsrecBuffer, data observe wrote
    while (1){
        char* dataObs = readBuffer(&buffer);
        printf("READING: This is what we read from buffer %s\n", dataObs);
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
                writeBuffer(&buffer, sample);
            }

            printf("NEXT READ INDEX: %d\n", buffer.latest);
        }
    }

    // write into the buffer, at the very end, the end marker
    writeBuffer(&buffer, "END_OF_DATA");

    pthread_exit(NULL);
} */

/* void *tapplot_function(void *arg){
    printf("Tapplot thread made!");
    Parcel *arguemnts = (Parcel *)arg;
    Buffer buffer = arguemnts->buffer;
    int arg3 = arguemnts->arg3;

    //printf("Tapplot BUFF: %p\n", &buffer);
    //printf("Tapplot BUFF ISASYNC: %d\n", buffer.isAsync);

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
      

    while (1){
        char* data = readBuffer(&buffer);
        // check for END marker symbolizing no more data to read
        if (strcmp(data, "END_OF_DATA") == 0) {
            break;
        }
        if (data != NULL) {
            processAndPlotData(data, arg3);            
        }
    }

    pthread_exit(NULL);
}
 */
/* // OBSERVE FUNCTIONS
struct PairList updateLastKnown(struct PairList pairlist, char * name, char * newVal) {
    for (int i = 0; i < pairlist.numPairs; i++) {
        if (strcmp(pairlist.pairs[i].name, name) == 0 && strcmp(pairlist.pairs[i].value, newVal) != 0) {
            strcpy(pairlist.pairs[i].value, newVal);
            return pairlist;
        }
    }
    strcpy(pairlist.pairs[pairlist.numPairs].name, name);
    strcpy(pairlist.pairs[pairlist.numPairs].value, newVal);
    pairlist.numPairs++;
    return pairlist;
}

char * getLastKnown(struct PairList pairlist, char * name) {
    for (int i = 0; i < pairlist.numPairs; i++) {
        if (strcmp(pairlist.pairs[i].name, name) == 0) {
            return pairlist.pairs[i].value;
        }
    }
    return "";
}


// parseData function
// splits a "name=value" string into a Pair struct.
void parseData(const char* data, Pair* outPair) {
    // up to 99 characters for name, up to 99 characters for value, and up to 1 character for the '='
    printf("TAPPLOT PARSING DATA: %s\n", data);

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
 */
/* void processAndPlotData(char * data, int index){
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

    gnuplot(NULL);
}

void gnuplot(void * arg) {
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
} */

