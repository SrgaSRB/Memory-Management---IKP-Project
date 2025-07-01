#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stddef.h>
#include <pthread.h> 

typedef struct BufferItem {
    int operation;
    size_t data;
} BufferItem;

typedef struct CircularBuffer {
    BufferItem *buffer;
    size_t head;
    size_t tail;
    size_t capacity;
    size_t count;
    
    pthread_mutex_t mutex;
    pthread_cond_t notFull;
    pthread_cond_t notEmpty;
} CircularBuffer;

CircularBuffer* init_buffer();
void write_buffer(CircularBuffer *cb, int operation, size_t data);
BufferItem read_buffer(CircularBuffer *cb);

void printBuffer(CircularBuffer *cb);
char* get_buffer_size(CircularBuffer *cb);
void destroy_buffer(CircularBuffer *cb);

#endif // CIRCULAR_BUFFER_H
