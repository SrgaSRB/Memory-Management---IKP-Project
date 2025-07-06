#include <stdio.h>
#include "../HeapManager/heap_manager_upgrade.h"
#include "../Buffer/circular_buffer.h"
#include "../ThreadPool/thread_pool.h"
#include "server.h"

int main() {
    initialize_heap();  

    CircularBuffer *cb = init_buffer();
    ThreadPool *pool = thread_pool_init(4);

    run_server(cb, pool);

    destroy_heap();
    destroy_buffer(cb);
    thread_pool_destroy(pool);

    return 0;
}
