#ifndef CLIENT
#define CLIENT

#include "../Buffer/circular_buffer.h"
#include <stdio.h>

void send_allocation_request(CircularBuffer *cb, size_t size);
void send_deallocation_request(CircularBuffer *cb);

#endif //CLIENT