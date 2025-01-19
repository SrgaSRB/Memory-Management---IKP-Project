#include "circular_buffer.h"
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define INIT_BUFFER_SIZE 10

typedef struct CircularBuffer
{
    size_t *buffer;
    size_t head;    // pointer for write
    size_t tail;    // pointer for read
    size_t capacity;
    size_t count;   // number of elements in buffer
    pthread_mutex_t mutex;
    pthread_cond_t notFull;
    pthread_cond_t notEmpty;

} CircularBuffer;

static CircularBuffer *cb;

void init_buffer(size_t size)
{
    cb = (CircularBuffer *)malloc(sizeof(CircularBuffer));
    cb->buffer = (size_t *)malloc(sizeof(size_t) * size);
    cb->head = 0;
    cb->tail = 0;
    cb->capacity = INIT_BUFFER_SIZE;
    cb->count = 0;
    pthread_mutexattr_init(&cb->mutex);
    pthread_cond_init(cb->notEmpty, NULL);
    pthread_cond_init(cb->notFull, NULL);
}

void expandBuffer(CircularBuffer *cb)
{
    int newCapacity = cb->capacity * 2;
    size_t *newBuffer = (size_t *)malloc(newCapacity);

    for (int i = 0; i < cb->count; i++)
    {
        newBuffer[i] = cb->buffer[(cb->tail + i) % cb->capacity];
    }
    free(cb->buffer);
    cb->capacity = newCapacity;
    cb->buffer = newBuffer;
    cb->head = cb->count;
    cb->tail = 0;

    printf("Buffer exend on capacity %d", cb->capacity);
}

void shrinkBuffer(CircularBuffer *cb)
{
    if (cb->count < INIT_BUFFER_SIZE)
        return;

    size_t newCapacity = cb->capacity / 2;
    size_t *newBuffer = (size_t *)malloc(sizeof(size_t) * newCapacity);

    for (int i = 0; i < cb->count; i++)
    {
        newBuffer[i] = cb->buffer[(cb->tail + i) % cb->capacity];
    }
    free(cb->buffer);
    cb->buffer = newBuffer;
    cb->capacity = newCapacity;
    cb->head = cb->count;
    cb->tail = 0;

    printf("Buffer shrunk to capacity %d\n", cb->capacity);
}

void write_buffer(CircularBuffer *cb, size_t data)
{
    pthread_mutex_lock(&cb->mutex);

    if (cb->count == cb->capacity)
    {
        expandBuffer(&cb);
    }

    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % cb->capacity;
    cb->count++;

    pthread_cond_signal(&cb->notEmpty);
    pthread_mutex_unlock(&cb->mutex);
}

void read_buffer(CircularBuffer *cb)
{
    pthread_mutex_lock(&cb->mutex);

    while (cb->count == 0)
    {
        pthread_cond_wait(&cb->notEmpty, &cb->mutex);
    }

    if (cb->count < cb->capacity / 3)
    {
        // umanji bafer za duplo
    }

    size_t data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->capacity;
    cb->count--;

    // pthread_cond_signal(&cb->notFull);
    pthread_mutex_unlock(&cb->mutex);

    // return data;
}

void destroy_buffer()
{
    free(cb->buffer);
    pthread_mutex_destroy(&cb->mutex);
}

void printBuffer(CircularBuffer *cb)
{
    printf("[ ");

    for (int i = 0; i < cb->count; i++)
    {
        if (cb->tail <= cb->head)
        {
            printf("%d, ", cb->buffer[cb->tail + i]);
        }
        else if (cb->tail > cb->head)
        {
            printf("%d, ", cb->buffer[(cb->tail + i) % cb->capacity]);
        }
        else
        {
            printf("., ");
        }
    }
}
