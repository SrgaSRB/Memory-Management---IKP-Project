#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

typedef void (*task_function_t)(void* arg);

typedef struct Task {
    task_function_t function;
    void* argument;
} Task;

typedef struct ThreadPool {
    pthread_t* threads;         // Niz niti
    Task* task_queue;           // Red zadataka
    size_t queue_size;          // Kapacitet reda
    size_t task_count;          // Broj zadataka u redu
    size_t head;                // Pokazivač na početak reda
    size_t tail;                // Pokazivač na kraj reda
    size_t num_threads;         // Broj niti u pool-u
    int stop;                   // Signal za zaustavljanje

    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_not_empty;
    pthread_cond_t queue_not_full;
} ThreadPool;

// Funkcije za upravljanje thread pool-om
ThreadPool* thread_pool_init(size_t num_threads);
void thread_pool_add_task(ThreadPool* pool, task_function_t function, void* arg);
void thread_pool_destroy(ThreadPool* pool);

#endif
