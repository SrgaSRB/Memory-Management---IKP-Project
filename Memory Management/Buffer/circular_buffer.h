#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stddef.h>

void init_buffer(size_t size);
void write_buffer(CircularBuffer *cb, size_t data);
void read_buffer(CircularBuffer *cb);
void destroy_buffer();
void printBuffer(CircularBuffer *cb);
size_t used_memory();

#endif CIRCULAR_BUFFER_H