#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_STR_LEN 1000
#define OBJECT_END_TOKEN 'e'

#define FALSE 0
#define TRUE 1

#define PATH_DELIMITER '|'

typedef enum 
{
    INTEGER = 'i',
    LIST = 'l',
    DICTIONARY = 'd',
    OTHER = 'o' // string or binary
} ObjType;

int is_digit(char c)
{
    return (c >= '0' && c <= '9');
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

int b_skip_obj(ObjType object_type, char* str)
{
    int offset = 0;
    if(object_type == OTHER)
    {
        // set offset to jump over the object

        char str_offset[MAX_STR_LEN];
        int j;
        for(j = 0; is_digit(str[offset]); j++)
        {
            str_offset[j] = str[offset];
            offset++;
        }
        str_offset[j] = '\0';

        int jump_offset = atoi(str_offset);
        offset += jump_offset; // skip the object
    }
    else if(object_type == INTEGER)
    {
        while(str[offset] != OBJECT_END_TOKEN)
        {
            offset++;
        }
    }

    return offset + 1; // skips OBJECT_END_TOKEN or ':'
}

// gets offset after the jump to the next object
// type - jump over every type but the specified
// if initially str is at the right type, go to the next obj of this type
int b_jump_to_type(ObjType type, char* str, int ignore_flag)
{
    int offset = 0;
    int nesting = 0; // example: d d (2x nesting) i (3x nesting) e e (1x nesting) e 
    // stop when the type is found and the nesting is 0, dont stop when ignoring
    while((str[offset] != '\0' && !(get_type(str[offset]) == type && nesting == 0)) || ignore_flag)
    {
        if(str[offset] == OBJECT_END_TOKEN)
        {
            nesting--;
            offset++;
        }
        else if(str[offset] == LIST || str[offset] == DICTIONARY)
        {
            nesting++;
            offset++;
        }
        else
        {
            offset += b_skip_obj(get_type(str[offset]), str + offset);
        }

        ignore_flag = 0; // ignore_flag only active on the first iteration
    }

    return offset;
}

// Gets offset of an object inside a list container
// str - bencoded str offseted
// we assume that the whole bencoded string is a list (usually with one element)
int b_get_list_offset(ObjType type, int count, char* str)
{
    int global_offset = 0;
    // index of an object in the list container
    for(int index = 0; str[global_offset] != '\0'; index++)
    {
        // set jump offset to nearest obj of type `type` on the current level of str[global_offset]
        int jump_offset = b_jump_to_type(type, &(str[global_offset]), (index == 0 ? TRUE : FALSE));

        ObjType current_type = get_type(str[global_offset]); 
        if(current_type != type)
        {
            global_offset += jump_offset; // jump
            jump_offset = b_jump_to_type(type, &(str[global_offset]), (index == 0 ? TRUE : FALSE)); // set new jump offset
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

int b_get_dict_offset(char* key, char* str)
{
    int offset = 0;
    int nesting = 0; // example: d d (2x nesting) i (3x nesting) e e (1x nesting) e 

    // stop when the key is found and nesting is at the level of beginning of str
    char current_key[MAX_STR_LEN] = "";

    int key_value_toggle = 0;

    for(int i = 0; i == 0 || (str[offset] != '\0' && !(strcmp(current_key, key) == 0 && nesting == 0)); i++)
    {
        if(str[offset] == OBJECT_END_TOKEN)
        {
            nesting--;
            offset++;
        }
        else if(str[offset] == LIST || str[offset] == DICTIONARY)
        {
            nesting++;
            offset++;
        }
        else
        {
            if(key_value_toggle == 0)
            {
                // get key
                char str_key_offset[MAX_STR_LEN];
                int j;
                for(j = 0; is_digit(str[offset]); j++)
                {
                    str_key_offset[j] = str[offset];
                    offset++;
                }
                str_key_offset[j] = '\0';

                int key_offset = atoi(str_key_offset);

                strncpy(current_key, str + offset + 1, key_offset); // + 1 -> jump over ':'
                current_key[key_offset] = '\0'; // null terminate (we assume that the whole str doesnt end with a key)

                offset += key_offset + 1; // skip the key and ':'
            }
            else
            {
                offset += b_skip_obj(get_type(str[offset]), str + offset);
            }

            if(nesting == 0)
            {
                key_value_toggle = (key_value_toggle ? 0 : 1);
            }
        }
    }

    return offset;
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
    // i++ to skip PATH_DELIMETER
    int i = 0;
    while(path[i] != '\0')
    {
        if(current_type == LIST)
        {
            char str_count[MAX_STR_LEN];
            int j;
            for(j = 0; is_digit(path[i+j]); j++)
            {
                str_count[j] = path[i+j];
            }
            str_count[j] = '\0';

            int count = atoi(str_count);

            i += j + 1; // skips the number and '.'

            current_type = (ObjType)path[i];

            i++; // skip object type
        
            str += b_get_list_offset(current_type, count, str); // go to the first object of the list
        }
        else if(current_type == DICTIONARY)
        {
            char key[MAX_STR_LEN];
            int j;
            for(j = 0; path[i+j] != PATH_DELIMITER && path[i+j] != '\0'; j++)
            {
                key[j] = path[i+j];
            }
            key[j] = '\0';

            i += j; // skips key

            str += b_get_dict_offset(key, str);
        }

        i++; // skip PATH_DELIMITER
    }

    printf("%s", str);
}

int main(int argc, char *argv[])
{
    char encoded_str[MAX_STR_LEN] = "d8:announce41:http://bttracker.debian.org:6969/announce7:comment35:'Debian CD from cdimage.debian.org'13:creation datei1573903810e9:httpseedsl145:https://cdimage.debian.org/cdimage/release/10.2.0//srv/cdbuilder.debian.org/dst/deb-cd/weekly-builds/amd64/iso-cd/debian-10.2.0-amd64-netinst.iso145:https://cdimage.debian.org/cdimage/archive/10.2.0//srv/cdbuilder.debian.org/dst/deb-cd/weekly-builds/amd64/iso-cd/debian-10.2.0-amd64-netinst.isoe4:infod6:lengthi351272960e4:name31:debian-10.2.0-amd64-netinst.iso12:piece lengthi262144e6:piecesee";
    //strcpy(encoded_str, argv[1]); 

    // int offset = b_get_list_offset(LIST, 0, encoded_str);
    // printf("%d %c\n", offset, encoded_str[offset]);

    b_get("0.d|info|piece length|", encoded_str, NULL);

    return 0;
}