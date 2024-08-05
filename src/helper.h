#pragma once

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

typedef unsigned char uchar;

/**
 * @brief Used for logging additional information inside deeper functions.
 * To enable uncomment `#define LOG(X) X`.
 * To disable comment `#define LOG(X) X`. 
 */
// #define LOG(X) X

#ifndef LOG
#   define LOG(X)
#endif

void print_nested(int nesting, const char *format, ...);

/**
 * @brief Generates n random bytes
 * 
 * @param n 
 * @param out 
 */
void generate_rand_str(int n, uchar *out);

/**
 * @brief Prints an errno error message and calls exit()
 * 
 * @param function_name 
 */
void failure(char *function_name);

/**
 * @brief Generates an explicit string (e.g. "\n\"test\"\t" -> "\\n\\\"test\\\"\\t")
 * 
 * @param src 
 * @param len 
 * @param dst in the worst case should be double the size of the src
 * @return int 
 */
int explicit_str(char *src, int len, char *dst);
