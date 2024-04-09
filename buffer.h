// old unfinished version
// bufferLib.h from Jake better approach (one struct)

#ifndef BUFFER.H
#define BUFFER.H

// struct to represent ring buffer
typedef struct {
    int *buffer;
    int size;
    int head;
    int tail;
    int count;
    // flag to indicate if all has been produces, ready to read
    int ready;
} RingBuffer;

// struct to represent 4-slot buffer
typedef struct {
    int buffer[4];
    int head;
    int tail;
    int count;
} SlotBuffer;

#endif // BUFFER.H

