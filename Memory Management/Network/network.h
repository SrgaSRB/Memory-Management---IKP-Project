#ifndef NETWORK_H
#define NETWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stddef.h>

// Definicija portova
#define SERVER_PORT 8080
#define CLIENT_PORT 8081
#define CLIENT_MONITORING_PORT 8082

int start_server(int port);
int start_client(int port, const char* ip);
int send_data(int socket, const void* data, size_t size);
int recive_data(int socket, void* buffer, size_t size);

#endif //NETWORK_H