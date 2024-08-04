#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helper.h"

#define MAX_BENCODED_TORRENT_LEN 1000000
#define MAX_BENCODED_INTEGER_LEN 100
#define MAX_BENCODED_OTHER_LEN 100000
#define MAX_BENCODE_NUM_LEN 20
#define MAX_BENCODE_KEY_LEN 100

#define MAX_BENCODE_PATH_COUNT_LEN 100

#define OBJECT_END_TOKEN 'e'

#define FALSE 0
#define TRUE 1

#define PATH_DELIMITER '|'
#define BENCODE_PRINT_LINE_LIMIT 100

typedef enum 
{
    INTEGER = 'i',
    LIST = 'l',
    DICTIONARY = 'd',
    OTHER = 'o' // string or binary
} ObjType;

int is_digit(uchar c);

ObjType get_type(uchar token);

int b_skip_obj(ObjType object_type, uchar* str);

int b_get_offset(uchar *path, uchar *str, int str_len);

int b_get(uchar *path, uchar *str, int str_len, uchar *out); // gets offset after the jump to the next object
// type - jump over every type but the specified
// if initially str is at the right type, go to the next obj of this type
int b_jump_to_type(ObjType type, uchar *str, int str_len, int ignore_flag);

// Gets offset of an object inside a list container
// str - bencoded str offseted
// we assume that the whole bencoded string is a list (usually with one element)
int b_get_in_list_offset(ObjType type, int count, uchar *str, int str_len);

int b_get_in_dict_offset(uchar *key, uchar *str, int str_len);

/*
list:
number.type

dictionary:
key

example of a nested integer path:
2.d/first/0.d/inner/0.l/3.i
*/

int b_print_tree(uchar *str, int str_len, int nesting, ObjType type);

void b_print(uchar *str, int str_len);

int b_insert_obj(uchar *str, int offset, uchar *str_obj, ObjType type);

void b_insert_element(uchar *str, int str_len, uchar *path, uchar *str_obj, ObjType type);

void b_insert_key_value(uchar *str, int str_len, uchar *path, uchar *key, uchar *value, ObjType value_type);

void b_insert_int(uchar *str, int str_len, uchar *path, int integer);

void b_create_list(uchar *list_out);

void b_create_dict(uchar *dict_out);