#include "server.h"
#include "../Network/network.h"
#include "../Buffer/circular_buffer.h"
#include "../ThreadPool/thread_pool.h"
#include "../HeapManager/heap_manager.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

typedef struct ClientArgs
{
    int client_socket;
    CircularBuffer *cb;
    ThreadPool *pool;
} ClientArgs;

typedef struct ReaderArgs
{
    CircularBuffer *cb;
    ThreadPool *pool;
} ReaderArgs;

static FILE *log_file = NULL;

void init_log_file()
{
    char filename[64];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(filename, sizeof(filename), "log_%Y%m%d_%H%M%S.txt", t);

    log_file = fopen(filename, "w");
    if (!log_file)
    {
        perror("Failed to create log file");
        exit(EXIT_FAILURE);
    }
}

void logMessage(const char *tag, const char *message)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", t);

    printf("[%s] [%s] %s\n", timebuf, tag, message);
    if (log_file)
    {
        fprintf(log_file, "[%s] [%s] %s\n", timebuf, tag, message);
        fflush(log_file);
    }
}

void process_task(void *arg)
{
    BufferItem *p = (BufferItem *)arg;
    BufferItem item = *p;
    free(p);

    char messengerBuffer[1024] = "\0";

    if (item.operation == 1)
    { // ALLOCATE
        void *mem = allocate_memory(item.data);
        snprintf(messengerBuffer, sizeof(messengerBuffer), "Allocated %zu bytes on memory address %p\n", item.data, mem);
    }
    else if (item.operation == 2)
    { // DEALLOCATE

        int retVal = free_memory((void *)item.data);

        if (retVal == 1)
        {
            snprintf(messengerBuffer, sizeof(messengerBuffer), "Deallocated memory at address %p\n", item.data, retVal); // Uredi format i ispis
        }
        else if (retVal == -1)
        {
            snprintf(messengerBuffer, sizeof(messengerBuffer), "Failed to deallocate memory at address %p (not allocated)\n", item.data);
        }
        else if (retVal == -2)
        {
            snprintf(messengerBuffer, sizeof(messengerBuffer), "Failed to deallocate memory at address %p (already freed)\n", item.data);
        }
        else
        {
            snprintf(messengerBuffer, sizeof(messengerBuffer), "Unknown error while deallocating memory at address %p\n", item.data);
        }
    }
    logMessage("HEAP", messengerBuffer);
}

void *buffer_reader(void *arg)
{
    ReaderArgs *reader_args = (ReaderArgs *)arg;
    CircularBuffer *cb = reader_args->cb;
    ThreadPool *pool = reader_args->pool;

    free(reader_args);

    while (1)
    {
        BufferItem item = read_buffer(cb);
        BufferItem *copy = malloc(sizeof(BufferItem));

        if (!copy)
        {
            perror("malloc failed");
            continue;
        }

        *copy = item;
        thread_pool_add_task(pool, process_task, copy);
    }

    return NULL;
}

void *handle_client(void *arg)
{
    ClientArgs *args = (ClientArgs *)arg;

    int client_socket = args->client_socket;
    CircularBuffer *cb = args->cb;
    ThreadPool *pool = args->pool;

    free(arg);

    char buffer[1024];
    char size_buffer[32];
    char address_buffer[32];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        if (recive_data(client_socket, buffer, sizeof(buffer)) <= 0)
        {
            logMessage("SERVER", "Client disconnected.");
            break;
        }

        if (!strcmp(buffer, "MONITORING") == 0)
        {
            logMessage("REQUEST", buffer);
        }
        
        if (strcmp(buffer, "ALLOCATE") == 0)
        {
            memset(size_buffer, 0, sizeof(size_buffer));
            recive_data(client_socket, size_buffer, sizeof(size_buffer));
            size_t size = strtoul(size_buffer, NULL, 10);

            if (size == 0)
            {
                logMessage("ERROR", "Invalid size received.");
                send_data(client_socket, "Invalid size\n.", 14);
                continue;
            }

            write_buffer(cb, 1, size);
            logMessage("BUFFER", "Memory allocation request written to buffer.");
            send_data(client_socket, "Success allocate\n.", 19);
        }
        else if (strcmp(buffer, "DEALLOCATE") == 0)
        {
            memset(address_buffer, 0, sizeof(address_buffer));
            recive_data(client_socket, address_buffer, sizeof(address_buffer));
            void *address = (void *)strtoull(address_buffer, NULL, 16);

            write_buffer(cb, 2, (size_t)address);
            logMessage("BUFFER", "Memory deallocation request written to buffer.");
            send_data(client_socket, "Success deallocate\n.", 21); // FIX: nije jos delocirana samo je stavljena u red zadelokaciju
        }
        else if (strcmp(buffer, "GET_HEAP") == 0)
        {
            char *overview = getHeapOverview();
            if (overview)
            {
                logMessage("BUFFER", "Heap overview sent to client.");
                send_data(client_socket, overview, strlen(overview));
            }
            else
            {
                logMessage("ERROR", "Failed to generate heap overview.");
                send_data(client_socket, "Error generating heap overview.\n", 32);
            }
        }
        else if (strcmp(buffer, "MONITORING") == 0)
        {
            char status_buffer[65536];

            char *bufferInfo = get_buffer_size(cb);
            // char *heapInfo = getHeapOverview(); // Kada radim sa testom, brzo se napuni buffer
            int activeWorkers = get_active_workers(pool);
            int completedTasks = get_executed_tasks(pool);
            int highActiveWorkers = get_high_active_workers(pool);

            // dodaj %s\n na pocetak stringa za ispis heap-a
            snprintf(status_buffer, sizeof(status_buffer),
                     "Buffer Info:\n%s\nActive Workers: %d (HIGH: %d)\nCompleted Tasks: %d\n",
                     // heapInfo ? heapInfo : "Heap Unavailable",
                     bufferInfo ? bufferInfo : "Unavailable",
                     activeWorkers, highActiveWorkers, completedTasks);

            send_data(client_socket, status_buffer, strlen(status_buffer));
            free(bufferInfo);
        }
        else
        {
            logMessage("ERROR", "Unknown request.");
            send_data(client_socket, "Unknown request\n.", 17);
        }
    }

    closesocket(client_socket);
    return NULL;
}

void run_server(CircularBuffer *cb, ThreadPool *pool)
{
    init_log_file();
    logMessage("SERVER", "Server started");

    int server_fd = start_server(8080);
    printf("Server is running on port 8080...\n");

    ReaderArgs *reader_args = malloc(sizeof(ReaderArgs));
    if (reader_args == NULL)
    {
        perror("Failed to allocate memory for reader args");
        closesocket(server_fd);
        exit(EXIT_FAILURE);
    }

    reader_args->cb = cb;
    reader_args->pool = pool;

    pthread_t reader_thread;
    pthread_create(&reader_thread, NULL, buffer_reader, reader_args);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

        if (client_socket < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("Client connected!\n");

        // Alociraj argumente za klijenta
        ClientArgs *args = malloc(sizeof(ClientArgs));
        if (!args)
        {
            perror("Failed to allocate memory for client args");
            closesocket(client_socket);
            continue;
        }
        args->client_socket = client_socket;
        args->cb = cb;
        args->pool = pool;

        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handle_client, args);
        pthread_detach(client_thread); // Detach kako ne bismo morali da čekamo niti
    }

    WSACleanup();
    closesocket(server_fd);
}
