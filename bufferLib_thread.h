#ifndef BUFFERLIB_THREAD
#define BUFFERLIB_THREAD_H

typedef struct {
    // Used for all bufs
    char ** data;
    int size;
    int isAsync;
    // Used only for ring bufs
    int in;
    int out;
    // Used only for async bufs
    int latest;
    int reading;
    int slots[2];
} Buffer;


typedef struct {
    Buffer *buffer;
    int arg3;
    char * fd;
} Parcel;

//init
void initBuffer(char * type, int size, int arg3, char * testFile, Parcel * parcel);

//writes
void asyncWrite (Buffer * buffer, char * item);
void ringWrite (Buffer * buffer, char * item);

//reads
char * asyncRead (Buffer * buffer);
char * ringRead (Buffer * buffer);

//wrapper funcs
char * readBuffer (Buffer * buffer);
void writeBuffer (Buffer * buffer, char * item);

#endif