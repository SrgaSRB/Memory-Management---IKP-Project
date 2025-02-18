#include "resource_manager.h"

static CircularBuffer *cb = NULL;

void initialize_resources() {
    initialize_heap();
    cb = init_buffer();
}

void destroy_resources() {
    destroy_buffer(cb);
    destroy_heap();
}

CircularBuffer* get_buffer() {
    return cb;
}
