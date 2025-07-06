#include "server.h"
#include "../Buffer/circular_buffer.h"
#include "../ThreadPool/thread_pool.h"
#include "../HeapManager/heap_manager.h"

#define THREAD_COUNT 8

int main() {
    // Inicijalizacija resursa
    initialize_heap();
    CircularBuffer *cb = init_buffer();
    ThreadPool *pool = thread_pool_init(THREAD_COUNT);

    // Pokretanje servera
    run_server(cb, pool);

    // Oslobađanje resursa
    destroy_buffer(cb);
    thread_pool_destroy(pool);
    destroy_heap();

    return 0;
}
