// This file really shouldn't be compiled/executed. Just a 
// place to throw in random tidbits of code. Make your own files
// to test any of this logic out.

// ######### RING BUFFER EXAMPLE FROM LAB REFERENCE ##################
/* 
Should be fine for tapper/tappet - create one ring buffer for each 'pipe' 
(i.e. Buf #1 = observe -> reconstruct, Buf #2 = reconstruct -> plot)    
*/

/* example of single producer and multiple consumers

   This uses a ring-buffer and no other means of mutual exclusion.
   It works because the shared variables "in" and "out" each
   have only a single writer.  It is an excellent technique for
   those situations where it is adequate.

   If we want to go beyond this, e.g., to handle multiple producers
   or multiple consumers, we need to use some more powerful means
   of mutual exclusion and synchronization, such as mutexes and
   condition variables.
   
 */

#define _XOPEN_SOURCE 500
#define _REENTRANT
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <values.h>
#include <errno.h>
#include <sched.h>

#define BUF_SIZE 10
int b[BUF_SIZE];
int in = 0, out = 0;

pthread_t consumer;
pthread_t producer;

void * consumer_body (void *arg) {
/* takes one unit of data from the buffer
   Assumes arg points to an element of the array id_number,
   identifying the current thread. */
  int tmp;
  int self = *((int *) arg);

  fprintf(stderr, "consumer thread starts\n"); 
  for (;;) {
     /* wait for buffer to fill */
     while (out == in);
     /* take one unit of data from the buffer */
     tmp = b[out];
     out = (out + 1) % BUF_SIZE;     
     /* copy the data to stdout */
     fprintf (stdout, "thread %d: %d\n", self, tmp);
  }
  fprintf(stderr, "consumer thread exits\n"); 
  return NULL;
}

void * producer_body (void * arg) {
/* create one unit of data and put it in the buffer */
   int i;
   fprintf(stderr, "producer thread starts\n"); 
   for (i = 0; i < MAXINT; i++) {
     /* wait for space in buffer */
     while (((in + 1) % BUF_SIZE) == out);
     /* put value i into the buffer */
     b[in] = i;
     in = (in + 1) % BUF_SIZE;     
  }
  return NULL;
}

int ring_buffer_main () {
   int result;
   pthread_attr_t attrs;

   /* use default attributes */
   pthread_attr_init (&attrs);

   /* create producer thread */
   if ((result = pthread_create (
          &producer, /* place to store the id of new thread */
          &attrs,
          producer_body,
          NULL))) {
      fprintf (stderr, "pthread_create: %d\n", result);
      exit (-1);
   } 
   fprintf(stderr, "producer thread created\n"); 

   /* create consumer thread */
   if ((result = pthread_create (
      &consumer,
      &attrs,
      consumer_body,
      NULL))) {
     fprintf (stderr, "pthread_create: %d\n", result);
     exit (-1);
   } 
   fprintf(stderr, "consumer thread created\n");

   /* give the other threads a chance to run */
   sleep (10);
   return 0;
}

// ################## 4-SLOT ASYNC LECTURE EXAMPLE ###################
/*
NOTE: Would likely need to use shmget / shmatt / etc. to acquire shared memory between processes!
Might be ok without it for tappet, 'buffer' here is a global variable that should be shared between threads.
*/

/* Copyright Rich West, Boston University, 2024

   This is an example producer-consumer thread program using Simpson's
   4-slot asynchronous buffering.

   It shows that by adjusting the execution times of the producer and
   consumer (effectively changing whether they are suitably
   rate-matched or not) leads to differing amounts of skipped (i.e.,
   lost) data. What is read, however, should remain the most recently
   observed values. 

   This is relevant to understanding how to correctly sample e.g.,
   sensor readings.

   Compile/link with: gcc -o 4-slot-threads 4-slot-threads.c -lpthread
   Test with: 4-slot-threads [integer-delay] > output_file
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_ITER 1000 /* Max iterations to produce/consume data. */

FILE *fp;

/* ######### Start of 4-slot buffering logic. ######### */
typedef int data_t;
# define EMPTY 0xFF

typedef enum {bit0=0, bit1=1} bit;
bit latest = 0, reading = 0;  // Control variables

// Initialize four data slots for items.
data_t buffer[2][2] = {{EMPTY, EMPTY}, {EMPTY, EMPTY}};
bit slot[2] = {0, 0};

void bufwrite (data_t item) {
  bit pair, index;
  pair = !reading;
  index = !slot[pair]; // Avoids last written slot in this pair, which reader may be reading.
  buffer[pair][index] = item; // Copy item to data slot.
  slot[pair] = index; // Indicates latest data within selected pair.
  latest = pair; // Indicates pair containing latest data.
}

data_t bufread (data_t buffer[2][2]) {
  bit pair, index;
  pair = latest; // Get pairing with newest info
  reading = pair; // Update reading variable to show which pair the reader is reading from.
  index = slot[pair]; // Get index of pairing to read from
  return (buffer[pair][index]); // Get desired value from the most recently updated pair.
}
/* ######### End of 4-slot buffering logic.  ##########*/


// Try changing DELAY between 1E4 and 1E8 to see changes in loss.
#define DELAY 1E4
unsigned int delay = DELAY;

/* Write data to 4-slot buffer. */
void put_data (void) {

  for (int i = 0; i < MAX_ITER; i++) {
    bufwrite (i);

    for (int j = 0; j < delay; j++); /* Add some delay. */
    sched_yield ();		     /* Probably only need for tappet: Allow another thread to run. */
  }
}

/* Read data from 4-slot buffer. */
void get_data (void) {

  static data_t old_value;
  data_t new_value;
  
  for (int i = 0; i < MAX_ITER; i++) {

    new_value = bufread (buffer);
    fprintf (fp, "Got data: %d\n", new_value); // Write to pipe.
    fflush (fp);
    
    printf ("Got data: %d ", new_value);       // And to stdout.
    fflush (stdout);
    
    if (new_value > old_value + 1)
      printf ("*********** Skipped data **********\n");
    else if (new_value == old_value)
      printf ("========== Unchanged data =========\n");
    else if (new_value < old_value)
      printf ("<<<<<<<<<<< Reverse data <<<<<<<<<<\n");
    else
      printf ("\n");

    old_value = new_value;

    for (int j = 0; j < delay; j++); /* Add some delay. */
    sched_yield ();		     /* Probably only needed for tappet: Allow another thread to run. */
  }
}


int four_slot_main (int argc, char *argv[]) {

  pthread_t producer, consumer;

  if (!(argc == 1 || argc == 2)) {
    fprintf (stderr, "Usage: %s [delay (1E4-1E8)]\n", argv[0]);
    exit (1);
  }
  else if (argc == 2) {
    delay = strtod (argv[1], NULL); // Works with command-line args with 'E' in them.
  }

  /* Create pipe to 4-slot Perl script for parsing data. This in turn
     pipes its output to gnuplot for live plotting. */
  fp = popen ("./4-slot.pl", "w");


  /* Now create a producer thread and a consumer thread that use a
     4-slot bufferto exchange data. */
  if (pthread_create (&producer, NULL,
		      (void *(*)(void *))put_data, NULL) != 0) {
    perror ("Unable to create thread");
    exit (1);
  }
  
  if (pthread_create (&consumer, NULL,
		      (void *(*)(void *))get_data, NULL) != 0) {
    perror ("Unable to create thread");
    exit (1);
  }

  /* Have the main thread wait for for the producer and consumer
     threads to complete their execution. */
  pthread_join (producer, NULL);
  pthread_join (consumer, NULL);

  /* Close the pipe created with popen. */
  pclose (fp);
  return 0;
  
}

// ############################ END ##################################

int main() {
    return 0;
}