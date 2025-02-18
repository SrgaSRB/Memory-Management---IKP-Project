#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "../Buffer/circular_buffer.h"
#include "../HeapManager/heap_manager.h"

void initialize_resources();
void destroy_resources();

CircularBuffer* get_buffer();
void* get_heap();

#endif
