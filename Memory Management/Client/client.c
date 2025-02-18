#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../Network/network.h"

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(EXIT_FAILURE);
    }

    int sock = start_client(SERVER_PORT, "127.0.0.1");
    if (sock < 0)
    {
        fprintf(stderr, "Failed to connect to server.\n");
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    while (1)
    {
        printf("Enter request \n1 -> ALLOCATE\n2 -> DEALLOCATE\n3 -> EXIT: ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Ukloni '\n'

        printf("%s\n", buffer);

        if (!strcmp(buffer, "1") && !strcmp(buffer, "2") && !strcmp(buffer, "3"))
        {
            printf("\nIncorrect entry!\n\n");
            continue;
        }

        if (!strcmp(buffer, "1"))
        {
            while (1)
            {

                send_data(sock, "ALLOCATE", strlen("ALLOCATE"));

                printf("Enter memory size for ALLOCATE: ");
                fgets(buffer, sizeof(buffer), stdin);
                buffer[strcspn(buffer, "\n")] = 0;

                // Validacija unosa veličine
                char *endptr;
                size_t size = strtoul(buffer, &endptr, 10);
                if (*endptr != '\0' || size == 0)
                {
                    printf("Invalid size entered!\n");
                    continue;
                }

                send_data(sock, buffer, strlen(buffer));
            }
        }
        else if (!strcmp(buffer, "2"))
        {
            send_data(sock, "DEALLOCATE", strlen("DEALLOCATE"));
        }
        else if (!strcmp(buffer, "3"))
        {
            send_data(sock, "EXIT", strlen("EXIT"));
            printf("Disconnected from server.\n");
            break;
        }
        else
        {
            memset(buffer, 0, sizeof(buffer));
            printf("\nIncorrect entry!\n\n");
            continue;
        }

        // Čitaj odgovor servera
        memset(buffer, 0, sizeof(buffer));
        recive_data(sock, buffer, sizeof(buffer));
        printf("Server response: %s\n", buffer);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
