#pragma once

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/**
 * @brief Generates n random bytes
 * 
 * @param n 
 * @param out 
 */
void generate_rand_str(int n, unsigned char *out);

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
