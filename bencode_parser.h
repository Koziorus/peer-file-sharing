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
int b_get_list_offset(ObjType type, int count, char* str);

int b_get_dict_offset(char* key, char* str);

/*
list:
number.type

dictionary:
key

example of a nested integer path:
2.d/first/0.d/inner/0.l/3.i
*/

void b_get(char* path, char* str, char* out);

int b_print(char* str, int nesting, ObjType type);