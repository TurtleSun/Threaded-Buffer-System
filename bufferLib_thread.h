#ifndef BUFFERLIB_THREAD
#define BUFFERLIB_THREAD_H

#define DELAY 5E4
#define MAX_STRING_LINES 100
#define MAX_STRINGS 100

typedef struct {
    // Used for all bufs
    char data[MAX_STRINGS][MAX_STRING_LINES];
    int size;
    int isAsync;
    int argn;
    // Used only for ring bufs
    int in;
    int out;
    // Used only for async bufs
    int latest;
    int reading;
    int slots[2];
} Buffer;

typedef struct {
    Buffer *readBuffer;
    Buffer *writeBuffer;
} Parcel;

//init
void initBuffer(char * type, int size, int argn, char * testFile, Buffer * buf);
void initParcel(Buffer* readBuffer, Buffer* writeBuffer, Parcel* parcel);

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