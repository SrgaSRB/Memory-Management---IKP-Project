#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <winsock2.h>
#include "../Network/network.h"

int main()
{

    int sock = start_client(SERVER_PORT, "127.0.0.1");
    if (sock < 0)
    {
        fprintf(stderr, "Failed to connect to server.\n");
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        send_data(sock, "ALLOCATE", strlen("ALLOCATE"));
        send_data(sock, "512", strlen("512"));
        Sleep(5);
    }

    closesocket(sock);
    return 0;
}