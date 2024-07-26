#include "helper.h"

// `out` should be allocated beforehand
// does not include null terminator
void generate_rand_string(int n, unsigned char* out)
{
    for(int i = 0; i < n; i++)
    {
        out[i] = rand() % 256; // random byte
    }
}