#include "network.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_STR_LEN 100

int main(int argc, char *argv[])
{
    char hostname[MAX_STR_LEN] = "example.com";
    char service[MAX_STR_LEN] = "80";

    // if(argc != 3)
    // {
    //     fprintf(stderr, "usage: %s hostname port\n", argv[0]);
    // }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* peer_address;
    if(getaddrinfo(hostname, service, &hints, &peer_address))
    {
        failure("getaddrinfo");
    }

    char address_buffer[100];
    char service_buffer[100];

    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer, sizeof(address_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST);

    printf("Remote address is: %s %s\n", address_buffer, service_buffer);    

    printf("Creating socket...\n");

    int socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype, peer_address->ai_protocol);

    if(socket_peer < 0)
    {   
        failure("socket");
    }

    printf("Connecting...\n");

    if(connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen) != 0)
    {
        failure("connect");
    }

    freeaddrinfo(peer_address);

    printf("Connected\n");

    printf("Send data:\n");

    fd_set sockets;

    FD_ZERO(&sockets);
    FD_SET(socket_peer, &sockets);
    FD_SET(fileno(stdin), &sockets);

    int max_socket = socket_peer + 1; // max socket + 1

    while(1)
    {
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 0.1s

        fd_set sockets_reads = sockets;
        if(select(max_socket, &sockets_reads, NULL, NULL, &timeout) == -1)
        {
            failure("select");
        }

        if(FD_ISSET(socket_peer, &sockets_reads))
        {
            char buffer[MAX_STR_LEN];
            int bytes_received = recv(socket_peer, buffer, MAX_STR_LEN, 0);
            if(bytes_received < 1)
            {
                printf("Connection closed\n");
                break;
            }

            printf("Received (%d) bytes:\n%.*s\n", bytes_received, bytes_received, buffer);
        }

        if(FD_ISSET(fileno(stdin), &sockets_reads))
        {
            char buffer[MAX_STR_LEN];
            char* fgets_ret = fgets(buffer, MAX_STR_LEN, stdin);
            if(fgets_ret == NULL)
            {
                break;
            }

            printf("\n");

            // -1 would mean that the socket has closed
            int bytes_sent = send(socket_peer, buffer, strlen(buffer), 0);

            printf("Sent (%d) bytes: \"%.*s\"\n", bytes_sent, bytes_sent, buffer);
        }
    }

    printf("Closing socket...\n");

    close(socket_peer);

    return 0;
}
