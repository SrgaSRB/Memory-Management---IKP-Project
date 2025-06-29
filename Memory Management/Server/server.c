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

typedef struct
{
    int client_socket;
    CircularBuffer *cb;
} ClientArgs;

void logMessage(const char *tag, const char *message)
{
    printf("[%s] %s\n", tag, message);
}

void process_task(void *arg)
{
    BufferItem item = *(BufferItem *)arg;

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
        }
        else if (retVal == -2)
        {
        }
    }
    logMessage("HEAP", messengerBuffer);
}

void *buffer_reader(void *arg)
{
    CircularBuffer *cb = ((CircularBuffer **)arg)[0];
    ThreadPool *pool = ((ThreadPool **)arg)[1];

    while (1)
    {
        BufferItem item = read_buffer(cb);
        thread_pool_add_task(pool, process_task, &item);
        Sleep(5000);
    }

    return NULL;
}

void *handle_client(void *arg)
{
    ClientArgs *args = (ClientArgs *)arg;
    int client_socket = args->client_socket;
    CircularBuffer *cb = args->cb;
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

        logMessage("REQUEST", buffer);

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
            // printBuffer(cb);
            send_data(client_socket, "Success allocate\n.", 19);
        }
        else if (strcmp(buffer, "DEALLOCATE") == 0)
        {
            memset(address_buffer, 0, sizeof(address_buffer));
            recive_data(client_socket, address_buffer, sizeof(address_buffer));
            void *address = (void *)strtoull(address_buffer, NULL, 16);

            write_buffer(cb, 2, (size_t)address);
            logMessage("BUFFER", "Memory deallocation request written to buffer.");
            // printBuffer(cb);
            send_data(client_socket, "Success deallocate\n.", 21); //FIX: nije jos delocirana samo je stavljena u red zadelokaciju
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
    int server_fd = start_server(8080);
    printf("Server is running on port 8080...\n");

    pthread_t reader_thread;
    pthread_create(&reader_thread, NULL, buffer_reader, (void *[]){cb, pool});

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

        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handle_client, args);
        pthread_detach(client_thread); // Detach kako ne bismo morali da čekamo niti
    }

    closesocket(server_fd);
}
