#pragma once

#define _POSIX_C_SOURCE 200809L // unlocks some functions (#ifdef)

#define MAX_STR_LEN 1000

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <byteswap.h>
#include <stdlib.h>
#include "helper.h"

// should be less than 20s
// in seconds
#define TCP_CONNECTION_TIMEOUT 5

// WARNING: timeout is a value, not a pointer
int connect_timeout(int fd, const struct sockaddr *addr, socklen_t len, struct timeval timeout);

int http_response_extract_body(unsigned char *http_data, int http_data_len, unsigned char *body);

void http_explicit_hex(unsigned char *data, int data_len, unsigned char *out);

int start_TCP_connection(char *remote_domain_name, char *remote_port, char *local_addr_out, char *local_port_out, int *socket_out);