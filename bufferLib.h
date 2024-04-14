/* #ifndef BUFFERLIB
#define BUFFERLIB .H

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

//init
//void initBuffer(Buffer * buf, int argc, char ** argv);

//writes
void asyncWrite (Buffer * buffer, char * item);
void ringWrite (Buffer * buffer, char * item);

//reads
char * asyncRead (Buffer * buffer);
char * ringRead (Buffer * buffer);

//wrapper funcs
char * readBuffer (Buffer * buffer);
void writeBuffer (Buffer * buffer, char * item);

#endif */