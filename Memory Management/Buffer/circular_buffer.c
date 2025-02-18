#include "circular_buffer.h"
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

#define INIT_BUFFER_SIZE (size_t)10

CircularBuffer *init_buffer()
{
    CircularBuffer *cb = (CircularBuffer *)malloc(sizeof(CircularBuffer));
    if (!cb)
    {
        fprintf(stderr, "Failed to allocate memory for CircularBuffer\n");
        exit(EXIT_FAILURE);
    }

    cb->buffer = (BufferItem *)malloc(sizeof(BufferItem) * INIT_BUFFER_SIZE);
    if (!cb->buffer)
    {
        free(cb);
        fprintf(stderr, "Failed to allocate memory for buffer\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < INIT_BUFFER_SIZE; i++)
    {
        cb->buffer[i].operation = -1; // Invalid operation
        cb->buffer[i].data = 0;
    }

    cb->head = 0;
    cb->tail = 0;
    cb->capacity = INIT_BUFFER_SIZE;
    cb->count = 0;
    pthread_mutex_init(&cb->mutex, NULL);
    pthread_cond_init(&cb->notEmpty, NULL);
    pthread_cond_init(&cb->notFull, NULL);

    printf("Init buffer success\n");
    return cb;
}

void expandBuffer(CircularBuffer *cb)
{
    int newCapacity = cb->capacity * 2;
    BufferItem *newBuffer = (BufferItem *)malloc(sizeof(BufferItem) * newCapacity);

    for (size_t i = 0; i < cb->count; i++)
    {
        newBuffer[i] = cb->buffer[(cb->tail + i) % cb->capacity];
    }
    free(cb->buffer);
    cb->capacity = newCapacity;
    cb->buffer = newBuffer;
    cb->head = cb->count;
    cb->tail = 0;

    printf("Buffer exend on capacity %zu", cb->capacity);
}

void shrinkBuffer(CircularBuffer *cb)
{
    if (cb->count < INIT_BUFFER_SIZE)
        return;

    size_t newCapacity = cb->capacity / 2;
    BufferItem *newBuffer = (BufferItem *)malloc(sizeof(BufferItem) * newCapacity);

    for (size_t i = 0; i < cb->count; i++)
    {
        newBuffer[i] = cb->buffer[(cb->tail + i) % cb->capacity];
    }
    free(cb->buffer);
    cb->buffer = newBuffer;
    cb->capacity = newCapacity;
    cb->head = cb->count;
    cb->tail = 0;

    printf("Buffer shrunk to capacity %zu\n", cb->capacity);
}

void write_buffer(CircularBuffer *cb, int operation, size_t data)
{
    pthread_mutex_lock(&cb->mutex);

    // Proširi bafer ako je pun
    if (cb->count == cb->capacity)
    {
        expandBuffer(cb);
    }

    cb->buffer[cb->head].operation = operation;
    cb->buffer[cb->head].data = data;

    cb->head = (cb->head + 1) % cb->capacity;
    cb->count++;

    pthread_cond_signal(&cb->notEmpty); // Signaliziraj da bafer više nije prazan
    pthread_mutex_unlock(&cb->mutex);

    //printf("[Write buffer]");
    //printBuffer(cb);
}

BufferItem read_buffer(CircularBuffer *cb)
{
    pthread_mutex_lock(&cb->mutex);

    while (cb->count == 0)
    {
        pthread_cond_wait(&cb->notEmpty, &cb->mutex);
    }

    if (cb->count < cb->capacity / 3)
    {
        shrinkBuffer(cb);
    }

    BufferItem data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->capacity;
    cb->count--;

    // pthread_cond_signal(&cb->notFull);
    pthread_mutex_unlock(&cb->mutex);

    //printf("[Read buffer]");
    //printBuffer(cb);

    return data;
}

void destroy_buffer(CircularBuffer *cb)
{
    free(cb->buffer);
    pthread_mutex_destroy(&cb->mutex);
    pthread_cond_destroy(&cb->notEmpty);
    pthread_cond_destroy(&cb->notFull);
}

void printBuffer(CircularBuffer *cb)
{
    pthread_mutex_lock(&cb->mutex);
    printf("\n[ ");

    for (size_t i = 0; i < cb->capacity; i++)
    {
        if (cb->count == cb->capacity)
        {
            // Buffer je pun
            printf("%zu, ", cb->buffer[i].data);
        }
        else if (cb->tail <= cb->head && i >= cb->tail && i < cb->head)
        {
            printf("%zu, ", cb->buffer[i].data);
        }
        else if (cb->tail > cb->head && (i >= cb->tail || i < cb->head))
        {
            printf("%zu, ", cb->buffer[i].data);
        }
        else
        {
            printf(". ");
        }
    }

    printf("] ");
    printf("Tail: %zu ", cb->tail);
    printf("Head: %zu ", cb->head);
    printf("Count: %zu ", cb->count);
    printf("Capacity: %zu \n\n", cb->capacity);

    pthread_mutex_unlock(&cb->mutex);
}
