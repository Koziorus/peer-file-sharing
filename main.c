#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bencode_parser.h"

int main(int argc, char *argv[])
{
    char encoded_str[MAX_STR_LEN] = "d1:Ali123e3:pore8:announce41:http://bttracker.debian.org:6969/announce7:comment35:'Debian CD from cdimage.debian.org'13:creation datei1573903810e4:infod6:lengthi351272960e4:name31:debian-10.2.0-amd64-netinst.iso12:piece lengthi262144e6:pieces3:ABCee";
    //strcpy(encoded_str, argv[1]); 

    // int offset = b_get_list_offset(LIST, 0, encoded_str);
    // printf("%d %c\n", offset, encoded_str[offset]);

    char obj[MAX_STR_LEN];
    b_get("0.d|A|0.l|0.o|", encoded_str, obj);
    printf("%s\n", obj);

    return 0;
}
