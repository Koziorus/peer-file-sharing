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

/**
 * @brief connect() with a specified timeout
 * waits until connected with custom timeout (if timeout is too high then the connect() may time out quicker)
normally connect() timeouts with a OS specified time (usually ~20s)
 * 
 * @param fd socket
 * @param addr 
 * @param len 
 * @param timeout 
 * @return int;
 * 0 on success;
 * -1 on error (errno);
 * -2 on timeout
 */
int connect_timeout(int fd, const struct sockaddr *addr, socklen_t len, struct timeval* timeout);

/**
 * @brief copies body of a http response
 * 
 * @param http_data input
 * @param http_data_len 
 * @param body 
 * @return int; 
 * 0 on success;
 * -1 on no body
 */
int http_response_extract_body(uchar *http_data, int http_data_len, uchar *body, int *body_len_out);

/**
 * @brief generates an explicit hexadecimal string from provided data
 * 
 * @param data input
 * @param data_len 
 * @param out 
 */
void http_explicit_hex(uchar *data, int data_len, uchar *out);

/**
 * @brief starts a TCP connection with specified connect timeout
 * 
 * @param remote_domain_name 
 * @param remote_port 
 * @param local_addr_out 
 * @param local_port_out 
 * @param socket_out 
 * @param timeout s
 * @return int;
 * 0 on success;
 * -1 on error;
 * other -> connect_timeout
 */
int start_TCP_connection(char *remote_domain_name, char *remote_port, char *local_addr_out, char *local_port_out, int *socket_out, struct timeval *timeout);