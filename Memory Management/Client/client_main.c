#include "../Network/network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    int client_socket = start_client(8080, "127.0.0.1");

    char buffer[1024];
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        printf("Enter request: 1 -> ALLOCATE, 2 -> DEALLOCATE, 3 -> EXIT, 4-> PRINT HEAP : ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Ukloni '\n'
        printf("Sending request: %s\n", buffer);

        if (strcmp(buffer, "1") == 0)
        {

            send_data(client_socket, "ALLOCATE", strlen("ALLOCATE"));

            printf("Enter memory size for ALLOCATE: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;

            char *endptr;
            size_t size = strtoul(buffer, &endptr, 10);

            if (*endptr != '\0' || size == 0)
            {
                printf("Invalid size entered! Try again.\n");
                continue;
            }

            send_data(client_socket, buffer, strlen(buffer));
            // memset(buffer, 0, sizeof(buffer)); // obrisati nakon ukidanja while petelje
        }
        else if (strcmp(buffer, "2") == 0)
        {
            send_data(client_socket, "DEALLOCATE", strlen("DEALLOCATE"));

            printf("Enter memory address to deallocate: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;

            send_data(client_socket, buffer, strlen(buffer));
        }
        else if (strcmp(buffer, "3") == 0)
        {
            send_data(client_socket, "EXIT", strlen("EXIT"));
            break;
        }
        else if (strcmp(buffer, "4") == 0)
        {
            send_data(client_socket, "GET_HEAP", strlen("GET_HEAP"));
        }
        else
        {
            printf("Invalid option.\n");
            continue;
        }

        // Primi odgovor servera

        recive_data(client_socket, buffer, sizeof(buffer));
        printf("Server response: %s\n", buffer);
    }

    closesocket(client_socket);
    return 0;
}
