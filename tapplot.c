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
    printf("argn: %d\n", argn);

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

    // 

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
void processAndPlotData(char* data, FILE* gnuplotPipe, int argn, int sampleNumber) {
    // take data that is in sample format and take only argn field for plot
    char* token = strtok(data, ",");
    // loop through data until we get to the field we want which is argn and then plot its value with the sample number
    // go through token until we get to index argn
    // then set Plotname to the name for that pair, as it is the arg we want to plot
    // plot that value with the sample number
    // plot the value of the field we want with the sample number
    fprintf(gnuplotPipe, "%d %s\n", sampleNumber,token); // value of the field we want); 
}


