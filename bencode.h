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

int is_digit(char c);

ObjType get_type(char token);

int b_skip_obj(ObjType object_type, char* str);

// gets offset after the jump to the next object
// type - jump over every type but the specified
// if initially str is at the right type, go to the next obj of this type
int b_jump_to_type(ObjType type, char* str, int ignore_flag);

// Gets offset of an object inside a list container
// str - bencoded str offseted
// we assume that the whole bencoded string is a list (usually with one element)
int b_get_in_list_offset(ObjType type, int count, char* str);

int b_get_in_dict_offset(char* key, char* str);

/*
list:
number.type

dictionary:
key

example of a nested integer path:
2.d/first/0.d/inner/0.l/3.i
*/

int b_get_offset(char *path, char *str);

void b_get(char *path, char *str, char *out);

int b_print_tree(char* str, int nesting, ObjType type);

void b_print(char *str);

int b_insert_obj(char *str, int offset, char *str_obj, ObjType type);

void b_insert_element(char *str, char *path, char *str_obj, ObjType type);

void b_insert_key_value(char *str, char *path, char *key, char *value, ObjType value_type);

void b_insert_int(char *str, char *path, int integer);

void b_create_list(char *list_out);

void b_create_dict(char *dict_out);