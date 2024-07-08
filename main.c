#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_STR_LEN 100
#define OBJECT_END_TOKEN 'e'

typedef enum 
{
    INTEGER = 'i',
    LIST = 'l',
    DICTIONARY = 'd',
    OTHER // string or binary
} ObjType;

int is_digit(char c)
{
    return (c >= '0' && c <= '9') || c == '-';
}

ObjType get_type(char token)
{
    if(is_digit(token))
    {
        return OTHER;
    }
    else
    {
        return (ObjType)token;
    }
}

// get offset after the jump over current object
int b_jump_over_int(ObjType type, char* str)
{
    
}

// gets offset after the jump to the next object
// type - jump over every type but the specified
// if initially str is at the right type, go to the next obj of this type
int b_jump_to(ObjType type, char* str, int ignore_flag)
{
    int offset = 0;
    int nesting = 0; // d d (2x nesting) i (3x nesting) e e (1x nesting) e 
    for(int i = 0; str[offset] != '\0' && (get_type(str[offset]) != type || nesting != 0) || ignore_flag; i++)
    {
        if(str[offset] == 'e')
        {
            nesting--;
            offset++;
        }
        else
        {
            ObjType object_type = get_type(str[offset]);
            if(object_type == OTHER)
            {
                // set offset to jump over the object

                char str_offset[MAX_STR_LEN];
                int j;
                for(j = 0; str[offset] >= '0' && str[offset] <= '9'; j++)
                {
                    str_offset[j] = str[offset];
                    offset++;
                }
                str_offset[j] = '\0';

                int jump_offset = atoi(str_offset);
                offset += 1 + jump_offset; // skip ':' and the object
            }
            else if(object_type == INTEGER)
            {
                while(str[offset] != OBJECT_END_TOKEN)
                {
                    offset++;
                }

                offset++; // skip OBJECT_END_TOKEN
            }
            else
            {
                nesting++;
                offset++;
            }
        }

        ignore_flag = 0; // ignore_flag only active on the first iteration
    }

    return offset;
}

// str - bencoded str offseted
// we assume that the whole bencoded string is a list (usually with one element)
int b_get_list_offset(ObjType type, int count, char* str)
{
    int global_offset = 0;
    for(int index = 0; str[global_offset] != '\0'; index++)
    {
        ObjType object_type = get_type(str[global_offset]); 

        // jump to nearest obj of type `type`
        int jump_offset = b_jump_to(type, &(str[global_offset]), (index == 0 ? 1 : 0));

        if(object_type != type)
        {
            global_offset += jump_offset; // jump
            jump_offset = b_jump_to(type, &(str[global_offset]), (index == 0 ? 1 : 0)); // set new jump offset
        }

        if(index == count)
        {
            return global_offset;
        }

        global_offset += jump_offset;
    }

    return -1;
}

int b_get_dict_offset(ObjType type, char key[MAX_STR_LEN])
{

}

int b_get_int(char bencoded_str[MAX_STR_LEN], char path[MAX_STR_LEN])
{

}

void b_get_other(char out[MAX_STR_LEN])
{

}

int main(int argc, char *argv[])
{
    char encoded_str[MAX_STR_LEN] = "dei34ed4:abcdd3:]]]ei55eei9eli3ee";
    //strcpy(encoded_str, argv[1]); 

    int offset = b_get_list_offset(LIST, 0, encoded_str);
    printf("%d %c\n", offset, encoded_str[offset]);

    return 0;
}