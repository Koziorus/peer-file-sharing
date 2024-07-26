#pragma once

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

void generate_rand_str(int n, unsigned char *out);

void failure(char *function_name);

int explicit_str(char *src, int len, char *dst);
