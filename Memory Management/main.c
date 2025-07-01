#include "Server/server.h"
#include "Buffer/circular_buffer.h"
#include "ThreadPool/thread_pool.h"
#include "HeapManager/heap_manager.h"
#include "../Network/network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void run_client() {
    int client_socket = start_client(8080, "127.0.0.1");

    char buffer[1024];
    while (1) {
        printf("Enter request: 1 -> ALLOCATE, 2 -> DEALLOCATE, 3 -> EXIT: ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Ukloni '\n'

        if (strcmp(buffer, "1") == 0) {
            send_data(client_socket, "ALLOCATE", strlen("ALLOCATE"));

            printf("Enter memory size to allocate: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;

            send_data(client_socket, buffer, strlen(buffer));
        } else if (strcmp(buffer, "2") == 0) {
            send_data(client_socket, "DEALLOCATE", strlen("DEALLOCATE"));

            printf("Enter memory address to deallocate: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;

            send_data(client_socket, buffer, strlen(buffer));
        } else if (strcmp(buffer, "3") == 0) {
            send_data(client_socket, "EXIT", strlen("EXIT"));
            break;
        } else {
            printf("Invalid option.\n");
        }

        // Primi odgovor servera
        memset(buffer, 0, sizeof(buffer));
        recive_data(client_socket, buffer, sizeof(buffer));
        printf("Server response: %s\n", buffer);
    }

    closesocket(client_socket);
}

int main() {
    
    int choice;
    printf("Choose mode:\n1 -> Run Server\n2 -> Run Client\n");
    scanf("%d", &choice);
    getchar(); // Consume newline character left by scanf

    if (choice == 1) {
        // Server logika
        initialize_heap();
        CircularBuffer *cb = init_buffer();
        ThreadPool *pool = thread_pool_init(4);

        run_server(cb, pool);

        destroy_buffer(cb);
        thread_pool_destroy(pool);
        destroy_heap();
    } else if (choice == 2) {
        // Klijent logika
        run_client();
    } else {
        printf("Invalid choice.\n");
    }

    return 0;
}
