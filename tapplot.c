// tapplot

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <stdio.h>
#include "bufferLib.h"
#include <unistd.h>
#include <stdbool.h>

#define PLOT_KEY 5678
#define SHMSIZE 100000
#define MAX_NAMES 100
#define MAX_NAME_LEN 50
#define MAX_VALUE_LEN 50

typedef struct {
    char name[MAX_NAME_LEN];
    char value[MAX_VALUE_LEN];
    // extra field to count appearances of each pair
    int count;
} Pair;

void processAndPlotData(char* data, FILE* gnuplotPipe, int argn, int sampleNumber);
void gnuplot(void * arg);
void parseData(char* data, Pair *outPair);

int main(int argc, char *argv[]) {
   // get argn from argv array passed in
    int argn = atoi(argv[6]);
    // print so i can make sure it is arg[6]
    //printf("argn: %d\n", argn);

    if (argn < 1 || argn > MAX_NAMES) {
        fprintf(stderr, "Field index %d is out of range\n", argn);
        return EXIT_FAILURE;
    }

    Buffer * shmBuffer = openBuffer(PLOT_KEY, SHMSIZE);

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
      

    int idx = -1;
    while (1){
        idx++;
        char* data = readBuffer(shmBuffer);
        // check for END marker symbolizing no more data to read
        if (strcmp(data, "END_OF_DATA") == 0) {
            break;
        }
        if (data != NULL) {
            processAndPlotData(data, gnuplotPipe, argn, idx);            
        }
    }

    // finalize Gnuplot plot
    // tell Gnuplot we're done sending data
    fprintf(gnuplotPipe, "e\n");
    fflush(gnuplotPipe);

    // close shared memory
    // detach it
    shmdt(shmBuffer);

    // close pipe to gnuplot
    pclose(gnuplotPipe);

    // exit
    fprintf(stderr, "TAPPLOT RETS\n");
    fflush(stderr);
    return 0;
}

// processAndPlotData function
void processAndPlotData(char* data, FILE* gnuplotPipe, int argn, int sampleNumber) {
    // Parse the data into name and value
    for (int i = 0; i < argn; i++) {
        data = strtok(data, ",");
    }
    Pair pair;
    parseData(data, &pair);

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
        exit(1);
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

void parseData(char* data, Pair *outPair) {
    // up to 99 characters for name, up to 99 characters for value, and up to 1 character for the '='
    sscanf(data, "%99[^=]=%99[^\n]", outPair->name, outPair->value);
}