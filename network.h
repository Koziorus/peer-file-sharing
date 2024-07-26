#pragma once

#define _POSIX_C_SOURCE 200809L // unlocks some functions (#ifdef)

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <byteswap.h>
#include <stdlib.h>

void failure(char *function_name);

int explicit_str(char *src, int len, char *dst);

int connect_timeout(int fd, const struct sockaddr *addr, socklen_t len, struct timeval timeout);

int http_get_body(unsigned char *http_data, int http_data_len, unsigned char *body);

void http_explicit_str(unsigned char *data, int data_len, unsigned char *out);
