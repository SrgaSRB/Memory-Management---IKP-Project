#include "heap_manager.h"
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define MAX_FREE_SEGMENTS 5
#define SEGMENT_SIZE 1024
#define INITIAL_SEGMENTS 5

typedef struct Segment
{
    void *memory;
    int is_free;
    struct Segment *next;
} Segment;

static Segment *head = NULL;
static pthread_mutex_t heap_mutex = PTHREAD_MUTEX_INITIALIZER;

void initialize_heap() // size_t segment_size, size_t initial_segments
{
    pthread_mutex_lock(&heap_mutex);
    for (size_t i = 0; i < INITIAL_SEGMENTS; i++)
    {
        Segment *new_segment = (Segment *)malloc(sizeof(Segment));
        if (!new_segment)
        {
            pthread_mutex_unlock(&heap_mutex);
            return; // Neuspešna alokacija
        }

        new_segment->memory = malloc(SEGMENT_SIZE);
        if (!new_segment->memory)
        {
            free(new_segment);
            pthread_mutex_unlock(&heap_mutex);
            return; // Neuspešna alokacija memorije
        }
        new_segment->is_free = 1;
        new_segment->next = head;
        head = new_segment;
    }
    pthread_mutex_unlock(&heap_mutex);
}

void* allocate_memory(size_t size)
{
    pthread_mutex_lock(&heap_mutex);
    Segment *current = head;

    while (current)
    {
        if (current->is_free && size <= SEGMENT_SIZE)
        {
            current->is_free = 0;
            pthread_mutex_unlock(&heap_mutex);
            return current->memory;
        }
        current = current->next;
    }

    Segment *newSegment = (Segment *)malloc(sizeof(Segment));
    if (!newSegment) {
        pthread_mutex_unlock(&heap_mutex);
        return NULL;  // Neuspešna alokacija
    }

    newSegment->memory = malloc(SEGMENT_SIZE);
    if (!newSegment->memory) {
        free(newSegment);
        pthread_mutex_unlock(&heap_mutex);
        return NULL;
    }
  
    newSegment->is_free = 0;
    newSegment->next = head;
    head = newSegment;

    pthread_mutex_unlock(&heap_mutex);
    return newSegment->memory;
}


void free_memory(void *ptr)
{
    pthread_mutex_lock(&heap_mutex);

    Segment *current = head;
    int free_segments = 0;
    int is_found = 0;

    // First fit
    while (current)
    {
        if (current->memory == ptr)
        {
            if (current->is_free) //memorija nije ni bila zauzeta
            {
                pthread_mutex_unlock(&heap_mutex);
                return;
            }
            current->is_free = 1;
            memset(current->memory, 0, SEGMENT_SIZE);
            is_found = 1;
            break;
        }
        if (current->is_free)
            free_segments++;

        current = current->next;
    }

    if (!is_found) //memorija nije nadjena sa datom adresom
    {
        pthread_mutex_unlock(&heap_mutex);
        return;
    }

    while (free_segments > MAX_FREE_SEGMENTS)
    {
        Segment *temp = NULL;
        current = head;

        while (current)
        {
            if (current->is_free)
            {
                if (temp == NULL)
                {
                    head = current->next;
                }
                else
                {
                    temp->next = current->next;
                }
                free(current->memory);
                free(current);
                free_segments--;
                break;
            }
            temp = current;
            current = current->next;
        }
    }

    pthread_mutex_unlock(&heap_mutex);
}

void destroy_heap()
{
    pthread_mutex_lock(&heap_mutex);
    Segment *current = head;
    Segment *next = NULL;

    while (current != NULL)
    {
        next = current->next;
        free(current->memory);
        free(current);
        current = next;
    }
    head = NULL;
    pthread_mutex_unlock(&heap_mutex);
}

void showAllocatedBlocks()
{
    printf("Allocated Addresses:\n");
    Segment *current = head;
    while (current != NULL)
    {
        if (!current->is_free) {
            printf("%p\n", current->memory);
        }
        current = current->next;
    }
}

/*
size_t used_memory() {
    pthread_mutex_lock(&heap_mutex);
    size_t used = 0;
    Segment *current = head;
    while (current) {
        if (!current->is_free) {
            used += current->size;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&heap_mutex);
    return used;
}
*/

