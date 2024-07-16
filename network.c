#include "network.h"

void failure(char* function_name)
{
    fprintf(stderr, "%s() failed (%d) -> [%s]\n", function_name, errno, strerror(errno));

    exit(1);
}