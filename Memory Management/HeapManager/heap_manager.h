#ifndef HEAP_MANAGER_H
#define HEAP_MANAGER_H

#include <stddef.h>

void* allocate_memory(size_t size);
int free_memory(void* ptr);
void initialize_heap();
void destroy_heap();
void showAllocatedBlocks();
char *getHeapOverview();

#endif // HEAP_MANAGER_H