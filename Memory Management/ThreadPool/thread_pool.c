#include "thread_pool.h"

// Funkcija niti za obradu zadataka
static void* thread_worker(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;

    while (1) {
        pthread_mutex_lock(&pool->queue_mutex);

        // Čekaj dok red nije prazan ili dok pool nije zaustavljen
        while (pool->task_count == 0 && !pool->stop) {
            pthread_cond_wait(&pool->queue_not_empty, &pool->queue_mutex);
        }

        if (pool->stop) {
            pthread_mutex_unlock(&pool->queue_mutex);
            pthread_exit(NULL);
        }

        // Uzimanje zadatka iz reda
        Task task = pool->task_queue[pool->head];
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->task_count--;

        // Signaliziraj da red više nije pun
        pthread_cond_signal(&pool->queue_not_full);
        pthread_mutex_unlock(&pool->queue_mutex);

        // Obrada zadatka
        task.function(task.argument);
    }

    return NULL;
}

// Inicijalizacija thread pool-a
ThreadPool* thread_pool_init(size_t num_threads) {
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (!pool) {
        fprintf(stderr, "Failed to allocate memory for thread pool\n");
        return NULL;
    }

    pool->num_threads = num_threads;
    pool->queue_size = num_threads * 2; // Red može imati više zadataka od broja niti
    pool->task_queue = (Task*)malloc(sizeof(Task) * pool->queue_size);
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);
    pool->task_count = 0;
    pool->head = 0;
    pool->tail = 0;
    pool->stop = 0;

    pthread_mutex_init(&pool->queue_mutex, NULL);
    pthread_cond_init(&pool->queue_not_empty, NULL);
    pthread_cond_init(&pool->queue_not_full, NULL);

    // Kreiranje niti
    for (size_t i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, thread_worker, pool);
    }

    return pool;
}

// Dodavanje zadatka u red
void thread_pool_add_task(ThreadPool* pool, task_function_t function, void* arg) {
    pthread_mutex_lock(&pool->queue_mutex);

    // Čekaj dok red nije pun
    while (pool->task_count == pool->queue_size) {
        pthread_cond_wait(&pool->queue_not_full, &pool->queue_mutex);
    }

    // Dodavanje zadatka u red
    pool->task_queue[pool->tail].function = function;
    pool->task_queue[pool->tail].argument = arg;
    pool->tail = (pool->tail + 1) % pool->queue_size;
    pool->task_count++;

    // Signaliziraj da red više nije prazan
    pthread_cond_signal(&pool->queue_not_empty);
    pthread_mutex_unlock(&pool->queue_mutex);
}

// Uništavanje thread pool-a
void thread_pool_destroy(ThreadPool* pool) {
    pthread_mutex_lock(&pool->queue_mutex);
    pool->stop = 1;

    // Signaliziraj svim nitima da se zaustave
    pthread_cond_broadcast(&pool->queue_not_empty);
    pthread_mutex_unlock(&pool->queue_mutex);

    // Čekaj da sve niti završe
    for (size_t i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->task_queue);
    free(pool->threads);
    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_not_empty);
    pthread_cond_destroy(&pool->queue_not_full);
    free(pool);
}
