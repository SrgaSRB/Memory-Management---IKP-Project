#ifndef HEAP_MANAGER_H
#define HEAP_MANAGER_H

#include <stddef.h>

void* allocate_memory(size_t size);
void free_memory(void* ptr);
void initialize_heap(size_t segment_size, size_t initial_segments);
void destroy_heap();
void showAllocatedBlocks();


#endif // HEAP_MANAGER_H