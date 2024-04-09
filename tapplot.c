// tapplot

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <bufferLib.h>

#define PLOT_KEY 5678
#define SHMSIZE 100000

int main(int argc, char *argv[]) {
    int argn = 1; // Default field index to plot
    if (argc > 1) {
        argn = atoi(argv[1]); // Get argn from command line argument
    }
    /////////////check where i passed it in


    // open shared memory that we initialized in tapper between reconstruct and tapplot
    int shm_Id_rectap = shmget(PLOT_KEY, SHMSIZE, 0666);
    if (shm_Id_rectap == NULL) {
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


    // read from shared memory
    // ... TODO: finish this
    while (1) {
        // if reading flag is 0 then the data is not ready to read
        while (!shmBuffer->reading) {
            // sleep briefly
            usleep(1000);
        }
        
        // reading from the buffer
        // reading from the buffer 
        while (true){
            char* data = readBuffer(obsrecBuffer);
            // check for END marker symbolizing no more data to read
            if (strcmp(data, "END_OF_DATA_YEET") == 0) {
                break;
            }
            if (data != NULL) {
                processAndPlotData(data);            }
        }
    }

    // close shared memory
    // detach it
    shmdt(shm_rectap_addr);

    // exit
    return 0;
}

// processAndPlotData function
