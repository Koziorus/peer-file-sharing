#include "helper.h"

void print_nested(int nesting, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    for(int i = 0; i < nesting; i++)
    {
        printf("\t");
    }

    vprintf(format, args);

    va_end(args);
}

// `out` should be allocated beforehand
// does not include null terminator
void generate_rand_str(int n, uchar* out)
{
    for(int i = 0; i < n; i++)
    {
        out[i] = rand() % 256; // random byte
    }
}

void failure(char* function_name)
{
    fprintf(stderr, "%s() failed (%d) -> [%s]\n", function_name, errno, strerror(errno));

    exit(1);
}

int explicit_str(char* src, int len, char* dst)
{
    char* escape_str[] = {"\\0", "\\a", "\\b", "\\e", "\\f", "\\n", "\\r", "\\t", "\\v", "\\\\", "\\'", "\\\"", "\\?"};
    char escape_char[] = "\0\a\b\e\f\n\r\t\v\\\'\"\?";
    int escape_len = sizeof(escape_char);

    int k = 0;
    for(int i = 0; i < len; i++)
    {
        int is_escape_char = 0;
        for(int j = 0; j < escape_len; j++)
        {
            if(src[i] == escape_char[j])
            {
                is_escape_char = 1;
                strcpy(dst + k, escape_str[j]);
                k++;
                break;
            }
        }

        if(!is_escape_char)
        {
            dst[k] = src[i];
        }

        k++;
    }

    return k;
}