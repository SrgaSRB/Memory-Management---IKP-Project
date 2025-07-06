#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <winsock2.h>
#include "../Network/network.h"

#define MONITORING_BUFFER 65536 // 2^16

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
        send_data(sock, "MONITORING", strlen("MONITORING"));
        char buffer[4096] = {0};

        recive_data(sock, buffer, sizeof(buffer));
        printf("== MONITORING INFO ==\n%s\n\n", buffer);
        Sleep(100);
    }

    closesocket(sock);
    return 0;
}