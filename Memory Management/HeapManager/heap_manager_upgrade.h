#ifndef HEAP_MANAGER_UPGRADE_H
#define HEAP_MANAGER_UPGRADE_H

#include <stddef.h>

// Alokacija i dealokacija
void initialize_heap(void);
void *allocate_memory(size_t size);
int free_memory(void *ptr);
void destroy_heap(void);

// Informacije i ispis
void showAllocatedBlocks(void);
char *getHeapOverview(void);

#endif // HEAP_MANAGER_UPGRADE_H
