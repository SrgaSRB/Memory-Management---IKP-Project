#ifndef SERVER_H
#define SERVER_H

#include "../Buffer/circular_buffer.h"
#include "../ThreadPool/thread_pool.h"

// Funkcija za pokretanje servera
void run_server(CircularBuffer *cb, ThreadPool *pool);

#endif
