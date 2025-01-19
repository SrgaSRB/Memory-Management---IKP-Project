#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define INITIAL_BUFFER_SIZE 5
#define PRODUCER_COUNT 2
#define MAX_CONSUMERS 32
#define MIN_CONSUMERS 1

typedef struct {
    int *buffer;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t notFull;
    pthread_cond_t notEmpty;
} CircularBuffer;

pthread_t consumerThreads[MAX_CONSUMERS];
int activeConsumers = 1;

// Inicijalizacija bafera
void initBuffer(CircularBuffer *cb, int size) {
    cb->buffer = (int *)malloc(sizeof(int) * size);
    cb->capacity = size;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    pthread_mutex_init(&cb->mutex, NULL);
    pthread_cond_init(&cb->notFull, NULL);
    pthread_cond_init(&cb->notEmpty, NULL);
}

// Prikaz stanja bafera
void printBuffer(CircularBuffer *cb) {

    printf("[ ");
    for (int i = 0; i < cb->capacity; i++) {
        if ((cb->tail <= cb->head && i >= cb->tail && i < cb->head) ||
            (cb->tail > cb->head && (i >= cb->tail || i < cb->head))) {
            printf("%d ", cb->buffer[i]);
        } else {
            printf(". ");
        }
    }
    printf("]  head:%d tail:%d count:%d\n", cb->head, cb->tail, cb->count);
}

void* consumer(void *arg);  // Dodaj ovu liniju pre funkcije addConsumer

// Dodavanje novih potrošača
void addConsumer(CircularBuffer *cb) {
    if (activeConsumers < MAX_CONSUMERS) {
        pthread_create(&consumerThreads[activeConsumers], NULL, consumer, cb);
        activeConsumers++;
        printf("Dodat potrošač. Aktivnih potrošača: %d\n", activeConsumers);
    }
}

// Proširenje bafera
void expandBuffer(CircularBuffer *cb) {
    int newCapacity = cb->capacity * 2;
    int *newBuffer = (int *)malloc(sizeof(int) * newCapacity);

    for (int i = 0; i < cb->count; i++) {
        newBuffer[i] = cb->buffer[(cb->tail + i) % cb->capacity];
    }

    free(cb->buffer);
    cb->buffer = newBuffer;
    cb->capacity = newCapacity;
    cb->head = cb->count;
    cb->tail = 0;

    printf("Bafer proširen na kapacitet: %d\n", cb->capacity);
    addConsumer(cb);  // Dodaj potrošača posle proširenja
}

// Upis u bafer
void writeBuffer(CircularBuffer *cb, int data) {
    pthread_mutex_lock(&cb->mutex);

    if (cb->count == cb->capacity) {
        expandBuffer(cb);
    }

    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % cb->capacity;
    cb->count++;

    printBuffer(cb);

    pthread_cond_signal(&cb->notEmpty);
    pthread_mutex_unlock(&cb->mutex);
}

// Čitanje iz bafera
int readBuffer(CircularBuffer *cb) {
    pthread_mutex_lock(&cb->mutex);

    while (cb->count == 0) {
        pthread_cond_wait(&cb->notEmpty, &cb->mutex);
    }

    int data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->capacity;
    cb->count--;

    pthread_cond_signal(&cb->notFull);
    pthread_mutex_unlock(&cb->mutex);

    return data;
}

// Proizvođač
void* producer(void *arg) {
    CircularBuffer *cb = (CircularBuffer *)arg;
    int data = 1;

    while (1) {
        writeBuffer(cb, data++);
        usleep(500000);  // Simulacija upisa
    }
    return NULL;
}

// Potrošač
void* consumer(void *arg) {
    CircularBuffer *cb = (CircularBuffer *)arg;

    while (1) {
        int data = readBuffer(cb);
        printf("Pročitano: %d\n", data);
        usleep(800000);  // Simulacija čitanja
    }
    return NULL;
}

// Pokretanje niti
int main() {
    CircularBuffer cb;
    initBuffer(&cb, INITIAL_BUFFER_SIZE);

    pthread_t producerThreads[PRODUCER_COUNT];

    // Pokretanje proizvođača
    for (int i = 0; i < PRODUCER_COUNT; i++) {
        pthread_create(&producerThreads[i], NULL, producer, &cb);
    }

    // Pokretanje jednog potrošača
    pthread_create(&consumerThreads[0], NULL, consumer, &cb);

    // Čekanje na proizvođače
    for (int i = 0; i < PRODUCER_COUNT; i++) {
        pthread_join(producerThreads[i], NULL);
    }

    // Čekanje na potrošače
    for (int i = 0; i < activeConsumers; i++) {
        pthread_join(consumerThreads[i], NULL);
    }

    free(cb.buffer);
    return 0;
}
