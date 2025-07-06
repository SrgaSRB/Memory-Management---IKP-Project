//  (4 "stripe"‑a = 4 nezavisne liste + mutexa)
#include "heap_manager_upgrade.h"
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define NUM_LOCKS 4               // koliko zona (stripe‑ova)
#define SEGMENT_SIZE 1024         // veličina svakog bloka (B)
#define INITIAL_SEGMENTS 5        // segmenta po zoni pri init‑u
#define MAX_FREE_SEGMENTS 5       // prag viška slobodnih
#define HEAP_OVERVIEW_BUFFER_SIZE 65536 // 64 KiB za getHeapOverview()

#define SEGMENTS_HIGH_LIMIT 1024
#define SEGMENTS_LOW_LIMIT 768

typedef enum
{
    FIRST_FIT,
    NEXT_FIT
} allocation_algorithm_t;


typedef struct Segment
{
    void *memory;
    int is_free;
    struct Segment *next;
} Segment;

static Segment *heap_heads[NUM_LOCKS] = {NULL};
static pthread_mutex_t heap_locks[NUM_LOCKS] = {
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

static size_t segment_count = 0; 

static Segment *last_allocated[NUM_LOCKS] = {NULL};
static allocation_algorithm_t g_algo = FIRST_FIT;

static inline void switch_allocation_algorithm(void)
{
    if (g_algo == FIRST_FIT && segment_count > SEGMENTS_HIGH_LIMIT)
        g_algo = NEXT_FIT;
    else if (g_algo == NEXT_FIT && segment_count < SEGMENTS_LOW_LIMIT)
        g_algo = FIRST_FIT;
}

static inline int stripe_index(void *ptr)
{
    return ((uintptr_t)ptr >> 4) % NUM_LOCKS; //poslednja 4 bita su uvek 0
}

void initialize_heap(void)
{
    for (int i = 0; i < NUM_LOCKS; i++)
    {
        pthread_mutex_lock(&heap_locks[i]);
        for (int j = 0; j < INITIAL_SEGMENTS; j++)
        {
            Segment *seg = malloc(sizeof(Segment));

            if (!seg)
            {
                pthread_mutex_unlock(&heap_locks[i]);
                fprintf(stderr, "Failed to allocate memory for segment\n");
                return;
            }

            seg->memory = malloc(SEGMENT_SIZE);

            if (!seg->memory)
            {
                free(seg);
                pthread_mutex_unlock(&heap_locks[i]);
                fprintf(stderr, "Failed to allocate memory for segment memory\n");
                return;
            }

            seg->is_free = 1;
            seg->next = heap_heads[i];
            heap_heads[i] = seg;
            segment_count++;
        }
        last_allocated[i] = heap_heads[i];

        pthread_mutex_unlock(&heap_locks[i]);
    }
    printf("[HEAP] Striped heap initialised (%d segments total)\n", (int)segment_count);
}

void *allocate_memory(size_t size)
{
    if (size > SEGMENT_SIZE)
        return NULL; 

    int start = rand() % NUM_LOCKS;

    for (int off = 0; off < NUM_LOCKS; off++)
    {
        int idx = (start + off) % NUM_LOCKS;
        pthread_mutex_lock(&heap_locks[idx]);

        Segment *start_seg;
        if (g_algo == NEXT_FIT && last_allocated[idx])
            start_seg = last_allocated[idx]->next ? last_allocated[idx]->next : heap_heads[idx];
        else
            start_seg = heap_heads[idx];

        Segment *cur = start_seg;
        do
        {
            if (cur && cur->is_free)
            {
                cur->is_free = 0;
                last_allocated[idx] = cur;
                pthread_mutex_unlock(&heap_locks[idx]);
                return cur->memory;
            }
            cur = cur ? cur->next : heap_heads[idx];
        } while (cur && cur != start_seg);

        pthread_mutex_unlock(&heap_locks[idx]);
    }

    int idx = start;
    pthread_mutex_lock(&heap_locks[idx]);

    Segment *seg = malloc(sizeof(Segment));
    if (!seg)
    {
        pthread_mutex_unlock(&heap_locks[idx]);
        return NULL;
    }

    seg->memory = malloc(SEGMENT_SIZE);
    if (!seg->memory)
    {
        free(seg);
        pthread_mutex_unlock(&heap_locks[idx]);
        return NULL;
    }

    seg->is_free = 0;
    seg->next = heap_heads[idx];
    heap_heads[idx] = seg;
    last_allocated[idx] = seg; // važno za NEXT_FIT

    segment_count++;
    switch_allocation_algorithm(); // globalna odluka

    pthread_mutex_unlock(&heap_locks[idx]);
    return seg->memory;
}

//  1  success
// -1  već slobodno
// -2  adresa nije pronađena
int free_memory(void *ptr)
{
    int idx = stripe_index(ptr); // određuje kojoj listi pripada adresa
    pthread_mutex_lock(&heap_locks[idx]);

    Segment *cur = heap_heads[idx];
    Segment *pre = NULL;
    int free_segments = 0;
    int found = 0;

    // Prva petlja: označavanje kao slobodno i brojanje slobodnih segmenata
    while (cur)
    {
        if (cur->memory == ptr)
        {
            if (cur->is_free)
            {
                pthread_mutex_unlock(&heap_locks[idx]);
                return -1; // već slobodno
            }
            cur->is_free = 1;
            memset(cur->memory, 0, SEGMENT_SIZE);
            found = 1;
        }
        if (cur->is_free)
            free_segments++;
        cur = cur->next;
    }

    if (!found)
    {
        pthread_mutex_unlock(&heap_locks[idx]);
        return -2; // adresa nije pronađena
    }

    // Druga petlja: oslobađanje viška segmenata (preko MAX_FREE_SEGMENTS)
    cur = heap_heads[idx];
    pre = NULL;
    while (cur && free_segments > MAX_FREE_SEGMENTS)
    {
        if (cur->is_free)
        {
            if (!pre)
                heap_heads[idx] = cur->next;
            else
                pre->next = cur->next;

            Segment *to_free = cur;
            cur = cur->next;

            free(to_free->memory);
            free(to_free);
            free_segments--;
            segment_count--; // statistika
            continue;
        }
        pre = cur;
        cur = cur->next;
    }

    switch_allocation_algorithm(); // proveri da li treba promeniti FF/NF

    pthread_mutex_unlock(&heap_locks[idx]);
    return 1;
}

void destroy_heap(void)
{
    for (int i = 0; i < NUM_LOCKS; i++)
    {
        pthread_mutex_lock(&heap_locks[i]);
        Segment *cur = heap_heads[i];
        while (cur)
        {
            Segment *nxt = cur->next;
            free(cur->memory);
            free(cur);
            cur = nxt;
        }
        heap_heads[i] = NULL;
        pthread_mutex_unlock(&heap_locks[i]);
    }
    segment_count = 0;
}

void showAllocatedBlocks(void)
{
    printf("Allocated addresses:\n");
    for (int i = 0; i < NUM_LOCKS; i++)
    {
        pthread_mutex_lock(&heap_locks[i]);
        for (Segment *cur = heap_heads[i]; cur; cur = cur->next)
            if (!cur->is_free)
                printf("[stripe %d] %p\n", i, cur->memory);
        pthread_mutex_unlock(&heap_locks[i]);
    }
}

char *getHeapOverview(void)
{
    static char buf[HEAP_OVERVIEW_BUFFER_SIZE];
    buf[0] = '\0';

    strcat(buf, "\nHeap overview (striped):\n");
    strcat(buf, "========================================\n");

    int idx_global = 0;
    for (int s = 0; s < NUM_LOCKS; s++)
    {
        pthread_mutex_lock(&heap_locks[s]);
        Segment *cur = heap_heads[s];
        while (cur)
        {
            char line[128];
            snprintf(line, sizeof(line), "| %3d | stripe %d | %p | %s |\n",
                     idx_global++, s, cur->memory, cur->is_free ? "FREE" : "ALLOC");
            strncat(buf, line, sizeof(buf) - strlen(buf) - 1);
            cur = cur->next;
        }
        pthread_mutex_unlock(&heap_locks[s]);
    }
    strcat(buf, "========================================\n");
    return buf;
}
