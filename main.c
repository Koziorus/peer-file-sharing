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
    OTHER = 'o' // string or binary
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

// gets offset after the jump to the next object
// type - jump over every type but the specified
// if initially str is at the right type, go to the next obj of this type
int b_jump_to(ObjType type, char* str, int ignore_flag)
{
    int offset = 0;
    int nesting = 0; // d d (2x nesting) i (3x nesting) e e (1x nesting) e 
    for(int i = 0; (str[offset] != '\0' && get_type(str[offset]) != type) || nesting != 0 || ignore_flag; i++)
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
            // when searching for a list or a dictionary point at the first element
            if(type == LIST || type == DICTIONARY)
            {
                global_offset++;
            }

            return global_offset;
        }

        global_offset += jump_offset;
    }

    return -1;
}

int b_get_dict_offset(ObjType type, char key[MAX_STR_LEN])
{

}

/*
list:
number.type

dictionary:
key

example of a nested integer path:
1.l/2.d/first/inner/0.d/in_dict/3.i
*/


//1.d/yky
void b_get(char path[MAX_STR_LEN], char* str, char* out)
{
    int current_type = LIST; // we assume the whole string is a big list
    for(int i = 0; path[i] != '\0'; i++)
    {
        if(current_type == LIST)
        {
            char str_count[MAX_STR_LEN];
            int j;
            for(j = 0; path[i+j] >= '0' && path[i+j] <= '9'; j++)
            {
                str_count[j] = path[i];
            }
            str_count[j] = '\0';

            int count = atoi(str_count);

            i += j + 1; // skips the number and '.'

            current_type = (ObjType)path[i];

            i++; // skip '/'
        
            str += b_get_list_offset(current_type, count, str); // go to the first object of the list
        }
    }

    printf("%s", str);
}

int main(int argc, char *argv[])
{
    char encoded_str[MAX_STR_LEN] = "l4:wert2:pll3:qweeei4444444444444edeee";
    //strcpy(encoded_str, argv[1]); 

    // int offset = b_get_list_offset(LIST, 0, encoded_str);
    // printf("%d %c\n", offset, encoded_str[offset]);

    b_get("0.l/0.l/0.i", encoded_str, NULL);

    return 0;
}