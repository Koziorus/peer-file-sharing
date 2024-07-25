#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BENCODED_TORRENT_LEN 1000000
#define MAX_BENCODED_INTEGER_LEN 100
#define MAX_BENCODED_OTHER_LEN 100000
#define MAX_BENCODE_NUM_LEN 20
#define MAX_BENCODE_KEY_LEN 100

#define MAX_BENCODE_PATH_COUNT_LEN 100

#define MAX_STR_LEN 10000
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

int is_digit(unsigned char c);

ObjType get_type(unsigned char token);

int b_skip_obj(ObjType object_type, unsigned char* str);

// gets offset after the jump to the next object
// type - jump over every type but the specified
// if initially str is at the right type, go to the next obj of this type
int b_jump_to_type(ObjType type, unsigned char* str, int ignore_flag);

// Gets offset of an object inside a list container
// str - bencoded str offseted
// we assume that the whole bencoded string is a list (usually with one element)
int b_get_in_list_offset(ObjType type, int count, unsigned char* str);

int b_get_in_dict_offset(unsigned char* key, unsigned char* str);

/*
list:
number.type

dictionary:
key

example of a nested integer path:
2.d/first/0.d/inner/0.l/3.i
*/

int b_get_offset(unsigned char *path, unsigned char *str);

void b_get(unsigned char *path, unsigned char *str, unsigned char *out);

int b_print_tree(unsigned char* str, int nesting, ObjType type);

void b_print(unsigned char *str);

int b_insert_obj(unsigned char *str, int offset, unsigned char *str_obj, ObjType type);

void b_insert_element(unsigned char *str, unsigned char *path, unsigned char *str_obj, ObjType type);

void b_insert_key_value(unsigned char *str, unsigned char *path, unsigned char *key, unsigned char *value, ObjType value_type);

void b_insert_int(unsigned char *str, unsigned char *path, int integer);

void b_create_list(unsigned char *list_out);

void b_create_dict(unsigned char *dict_out);