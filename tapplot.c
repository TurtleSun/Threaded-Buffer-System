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


void processAndPlotData(char* data, FILE* gnuplotPipe, int argn, int sampleNumber);

#define PLOT_KEY 5678
#define SHMSIZE 100000
#define MAX_NAMES 100

int main(int argc, char *argv[]) {
   // get argn from argv array passed in
    int argn = atoi(argv[6]);
    // print so i can make sure it is arg[6]
    //printf("argn: %d\n", argn);

    if (argn < 1 || argn > MAX_NAMES) {
        fprintf(stderr, "Field index %d is out of range\n", argn);
        return EXIT_FAILURE;
    }

    // open shared memory that we initialized in tapper between reconstruct and tapplot
    int shm_Id_rectap = shmget(PLOT_KEY, SHMSIZE, 0666);
    if (shm_Id_rectap == -1) {
        // something went horribly wrong
        perror("shmatt error");
        exit(1);
    }

    void * shm_rectap_addr = shmat(shm_Id_rectap, NULL, 0);
    if (shm_rectap_addr == NULL) {
        perror("shmat failed for tapplot");
        exit(1);
    }

    Buffer *shmBuffer = (Buffer *)shm_rectap_addr;

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
    fprintf(gnuplotPipe, "plot '-' using 1:2 with lines title 'Field %d'\n", argn);


    // READING FROM RECONSTRUCT BUFFER
    // if reading flag is 0 then the data is not ready to read
    while (!shmBuffer->reading) {
        // sleep briefly
        usleep(1000);
    }

    // keep track of sample number
    int sampleNumber = 0;

    // reading from the buffer 
    while (true){
        char* data = readBuffer(shmBuffer);
        // check for END marker symbolizing no more data to read
        if (strcmp(data, "END_OF_DATA_YEET") == 0) {
            break;
        }
        if (data != NULL) {
            processAndPlotData(data, gnuplotPipe, argn, sampleNumber++);
        }
    }

    // finalize Gnuplot plot
    // tell Gnuplot we're done sending data
    fprintf(gnuplotPipe, "e\n");
    fflush(gnuplotPipe);

    // close shared memory
    // detach it
    shmdt(shm_rectap_addr);

    // close pipe to gnuplot
    pclose(gnuplotPipe);

    // exit
    return 0;
}

// processAndPlotData function
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

