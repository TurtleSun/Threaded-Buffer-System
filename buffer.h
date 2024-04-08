#ifndef BUFFER.H
#define BUFFER.H

// struct to represent ring buffer
typedef struct {
    int *buffer;
    int size;
    int head;
    int tail;
    int count;
} ring_buffer;

// struct to represent 4-slot buffer
typedef struct {
    int buffer[4];
    int head;
    int tail;
    int count;
} slot_buffer;

#endif // BUFFER.H

