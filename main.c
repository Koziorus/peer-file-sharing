#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bencode_parser.h"

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

void b_insert_int()
{

}

void b_insert_list()
{

}

void b_insert_dict()
{

}

int main(int argc, char *argv[])
{
    char encoded_str[MAX_STR_LEN] = "d8:announce41:http://bttracker.debian.org:6969/announce7:comment35:'Debian CD from cdimage.debian.org'13:creation datei1573903810e4:infod6:lengthi351272960e4:name31:debian-10.2.0-amd64-netinst.iso12:piece lengthi262144e6:pieces3:ABCee";
    //strcpy(encoded_str, argv[1]); 

    // int offset = b_get_list_offset(LIST, 0, encoded_str);
    // printf("%d %c\n", offset, encoded_str[offset]);

    // b_print(encoded_str, 0, LIST);
    // printf("\n");

    // char obj[MAX_STR_LEN];
    // b_get("0.d|", encoded_str, obj);
    // printf("%s\n", obj);

    char b_info_str[MAX_STR_LEN] = "de";

    b_insert_key_value(b_info_str, "0.d|", "length", "567", OTHER);
    b_insert_key_value(b_info_str, "0.d|", "name", "'debian'", OTHER);

    char b_str[MAX_STR_LEN] = "de";

    b_insert_key_value(b_str, "0.d|", "announce", "hhtp", OTHER);
    b_insert_key_value(b_str, "0.d|", "info", b_info_str, DICTIONARY);
    
    b_print(b_str);    

    // printf("%s", b_str);

    return 0;
}
