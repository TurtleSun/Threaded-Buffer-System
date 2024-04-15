#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "bufferLib_thread.h"
#include "reconstruct_thread.h"
#include <pthread.h>

void *tapplot_function(void *arg);
void processAndPlotData(char* data, FILE* gnuplotPipe, int argn, int sampleNumber);
//void gnuplot(void * arg);

void *tapplot_function(void *arg){
    printf("Tapplot thread made! \n");
    Parcel *arguemnts = (Parcel *)arg;
    Buffer *buffer = arguemnts->buffer;
    int arg3 = arguemnts->arg3;

    //printf("Observe BUFF: %p\n", &buffer);
    printf("CHECKING: Tapplot BUFF ISASYNC: %d\n", buffer->isAsync);

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
        char* data = readBuffer(buffer);
        printf("TAPPLOT: Here is what the buffer game me: %s\n", data);
        // check for END marker symbolizing no more data to read
        if (strcmp(data, "END_OF_DATA") == 0) {
            break;
        }
        if (data != NULL) {
            processAndPlotData(data, gnuplotPipe, arg3, idx);            
        }
    }

    // finalize Gnuplot plot
    // tell Gnuplot we're done sending data
    fprintf(gnuplotPipe, "e\n");
    fflush(gnuplotPipe);

    pclose(gnuplotPipe);

    pthread_exit(NULL);
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

    //gnuplot(NULL);
}

/*
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
}*/