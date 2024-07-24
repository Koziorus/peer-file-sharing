#include "bencode.h"

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
int b_get_in_list_offset(ObjType type, int count, char* str)
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
            return global_offset;
        }

        global_offset += jump_offset;
    }

    return -1;
}

int b_get_in_dict_offset(char* key, char* str)
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
            if(nesting == 0)
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

                key_value_toggle = (key_value_toggle ? 0 : 1);
            }
            else
            {
                offset += b_skip_obj(get_type(str[offset]), str + offset);
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


int b_get_offset(char* path, char* str)
{
    int offset = 0; // string offset

    int container_type = LIST; // we assume the whole string is a big list
    // i++ to skip PATH_DELIMETER
    int i = 0;
    while(i < strlen(path))
    {
        if(container_type == LIST)
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

            ObjType object_type = (ObjType)path[i];
            
            i++; // skip object type

            offset += b_get_in_list_offset(object_type, count, str + offset); // go to the object_type

            // if its a container and we expect another nesting
            if(i < strlen(path) && (object_type == LIST || object_type == DICTIONARY))
            {
                offset++; // get to the first object of the container (skip type token)
            }
        
            container_type = object_type;
        }
        else if(container_type == DICTIONARY)
        {
            char key[MAX_STR_LEN];
            int j;
            for(j = 0; path[i+j] != PATH_DELIMITER && path[i+j] != '\0'; j++)
            {
                key[j] = path[i+j];
            }
            key[j] = '\0';

            i += j; // skips key

            offset += b_get_in_dict_offset(key, str + offset);

            container_type = LIST; // value is assumed to be a list
        }

        i++; // skip PATH_DELIMITER
    }

    return offset;
}

void b_get(char* path, char* str, char* out)
{
    int offset = b_get_offset(path, str);

    int obj_len;
    if(get_type(str[offset]) == INTEGER)
    {
        int integer_ptr = 1; // skip INTEGER type token
        while(str[offset + integer_ptr] != OBJECT_END_TOKEN)
        {
            integer_ptr++;
        }

        obj_len = integer_ptr - 1; // -1 -> without OBJECT_END_TOKEN

        offset++; // skip OBJECT_END_TOKEN
    }
    else if(get_type(str[offset]) == OTHER)
    {
        char str_obj_len[MAX_STR_LEN];
        int j;
        for(j = 0; is_digit(str[offset]); j++)
        {
            str_obj_len[j] = str[offset];
            offset++;
        }
        str_obj_len[j] = '\0';

        obj_len = atoi(str_obj_len);

        offset++; // skip ':'
    }
    else if(get_type(str[offset] == LIST || get_type(str[offset] == DICTIONARY)))
    {
        obj_len = 1; // skip container type token
        int nesting = 0; 
        // while in the current dictionary (nesting == 0)
        while((str[offset + obj_len] != '\0' && nesting >= 0))
        {
            if(str[offset + obj_len] == OBJECT_END_TOKEN)
            {
                nesting--;
                
                obj_len++;
            }
            else if(str[offset + obj_len] == LIST || str[offset + obj_len] == DICTIONARY)
            {
                nesting++;
                obj_len++;
            }
            else
            {
                obj_len += b_skip_obj(get_type(str[offset + obj_len]), str + offset + obj_len);
            }
        }
    }

    strncpy(out, str + offset, obj_len);
    out[obj_len] = '\0';
}

void b_print_nesting(int nesting)
{
    for(int j = 0; j < nesting; j++)
    {
        printf("  ");
    }
}

int b_print_list(char* str, int nesting)
{
    int i = 0;
    ObjType list_obj_type;
    while(str[i] != '\0')
    {
        list_obj_type = get_type(str[i]);
        
        if(str[i] == OBJECT_END_TOKEN)
        {
            return i + 1;
        }
        else
        {
            if(i != 0)
            {
                printf("\n");
            }

            if(list_obj_type == OTHER)
            {

                // TODO: replace those kind of pieces of code with read_number()
                char str_obj_len[MAX_STR_LEN];
                int j;
                for(j = 0; is_digit(str[i]); j++)
                {
                    str_obj_len[j] = str[i];
                    i++;
                }
                str_obj_len[j] = '\0';

                int obj_len = atoi(str_obj_len);

                b_print_nesting(nesting);

                printf("%s:", str_obj_len);

                i++; // skip ':'

                while(obj_len--)
                {
                    printf("%c", str[i]);
                    i++;
                }
            }
            else 
            {
                i++;
                i += b_print_tree(str + i, nesting, list_obj_type);
            }
        }
    }

    return i;
}

int b_print_dict(char* str, int nesting)
{
    int i = 0;
    ObjType list_obj_type;

    int key_value_toggle = 0;

    while(TRUE)
    {
        list_obj_type = get_type(str[i]);
        if(str[i] == OBJECT_END_TOKEN)
        {
            return i + 1;
        }
        else
        {
            if(i != 0)
            {
                printf("\n");
            }

            if(list_obj_type == OTHER)
            {
                char str_obj_len[MAX_STR_LEN];
                int j;
                for(j = 0; is_digit(str[i]); j++)
                {
                    str_obj_len[j] = str[i];
                    i++;
                }
                str_obj_len[j] = '\0';

                int obj_len = atoi(str_obj_len);

                b_print_nesting(nesting + key_value_toggle);

                printf("%s:", str_obj_len);

                i++; // skip ':'

                while(obj_len--)
                {
                    printf("%c", str[i]);
                    i++;
                }
            }
            else
            {
                i++;
                i += b_print_tree(str + i, nesting + key_value_toggle, list_obj_type);
            }

            key_value_toggle = (key_value_toggle ? 0 : 1);
        }
    }

    return i;
}

int b_print_integer(char* str, int nesting)
{
    b_print_nesting(nesting);

    char str_int[MAX_STR_LEN];

    int i = 0;
    while(str[i] != OBJECT_END_TOKEN)
    {
        printf("%c", str[i]);
        i++;
    }

    return i + 1; // + 1 -> skip OBJECT_END_TOKEN
}

int b_print_tree(char* str, int nesting, ObjType type)
{
    char print[MAX_STR_LEN];

    int i = 0;

    b_print_nesting(nesting);
    printf("%c\n", type);
    if(type == LIST)
    {
        i += b_print_list(str + i, nesting + 1);
    }
    else if(type == DICTIONARY)
    {
        i += b_print_dict(str + i, nesting + 1);
    }
    else if(type == INTEGER)
    {
        i += b_print_integer(str + i, nesting + 1);
    }

    printf("\n");
    b_print_nesting(nesting);
    printf("%c", OBJECT_END_TOKEN);

    return i;
}

void b_print(char* str)
{
    if(str[0] == '\0')
    {
        return;
    }

    b_print_tree(str, 0, LIST);
    printf("\n");
}

// returns offset to the first character after the inserted object
int b_insert_obj(char* str, int offset, char* str_obj, ObjType type)
{
    char str_A[MAX_STR_LEN] = "";
    char str_B[MAX_STR_LEN] = "";
    strncpy(str_A, str, offset); // abcdefgh
    str_A[offset] = '\0';
    strncpy(str_B, str + offset, strlen(str) - offset);
    str_B[strlen(str) - offset] = '\0';

    char str_whole_obj[MAX_STR_LEN];

    if(type == LIST || type == DICTIONARY || type == INTEGER)
    { 
        sprintf(str_whole_obj, "%s", str_obj);
    }
    else
    {
        sprintf(str_whole_obj, "%d:%s", strlen(str_obj), str_obj);
    }

    sprintf(str, "%s%s%s", str_A, str_whole_obj, str_B);

    int new_offset = offset + strlen(str_whole_obj);
    return new_offset; 
}

// element is an object inside a list
void b_insert_element(char* str, char* path, char* str_obj, ObjType type)
{
    int insert_offset = b_get_offset(path, str);
    b_insert_obj(str, insert_offset, str_obj, type);
}

void b_insert_key_value(char* str, char* path, char* key, char* value, ObjType value_type)
{
    int insert_offset = b_get_offset(path, str);
    insert_offset = b_insert_obj(str, insert_offset, key, OTHER);
    b_insert_obj(str, insert_offset, value, value_type);
}

void b_insert_int(char* str, char* path, int integer)
{
    int insert_offset = b_get_offset(path, str);

    char str_int[MAX_STR_LEN];
    sprintf(str_int, "i%de", integer);
    b_insert_obj(str, insert_offset, str_int, INTEGER);
}

void b_create_list(char* list_out)
{
    strcpy(list_out, "le");
}

void b_create_dict(char* dict_out)
{
    strcpy(dict_out, "de");
}
