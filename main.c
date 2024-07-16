#define _POSIX_C_SOURCE 200809L // unlocks some functions (#ifdef)

#include <stdio.h>
#include <string.h>
#include "bencode.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>

void failure(char* function_name)
{
    fprintf(stderr, "%s() failed (%d) -> [%s]\n", function_name, errno, strerror(errno));

    exit(1);
}

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;

    getaddrinfo(0, "8080", &hints, &bind_address);

    int socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);

    if(socket_listen < 0)
    {
        failure("socket");
    }

    int option = 0;
    if(setsockopt(socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&option, sizeof(option)) != 0)
    {
        failure("setsockopt");
    }

    if(bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen) != 0)
    {
        failure("bind");
    }
    freeaddrinfo(bind_address);

    if(listen(socket_listen, 10) < 0)
    {
        failure("listen");
    }

    printf("Waiting for connections...\n");

    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);

    int socket_client = accept(socket_listen, (struct sockaddr*) &client_address, &client_len);

    if(socket_client < 0)
    {
        failure("accept");
    }

    printf("Client connected...\n");

    char address_buffer[100];
    getnameinfo((struct sockaddr*)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);

    printf("%s\n", address_buffer);

    char request[1024];
    int bytes_received = recv(socket_client, request, 1024, 0);
    printf("Received %d bytes\n", bytes_received);

    printf("%.*s\n", bytes_received, request);

    printf("Sending response...\n");

    char* response =
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\n"
    "Content-Type: text/plain\r\n\r\n"
    "Local time is: ";
    int bytes_sent = send(socket_client, response, strlen(response), 0);

    printf("Sent %d of %d bytes\n", bytes_sent, (int)strlen(response));

    time_t timer;
    time(&timer);
    char* time_msg = ctime(&timer);
    bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);

    printf("Sent %d of %d bytes\n", bytes_sent, (int)strlen(time_msg));

    printf("Closing connection...\n");
    close(socket_client);

    printf("Closing listening socket...\n");

    close(socket_listen);


    return 0;
}

    