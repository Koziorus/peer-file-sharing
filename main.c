#include "bencode.h"
#include <openssl/sha.h>

struct BencodeInfo
{
    int length;
    char name[MAX_STR_LEN];
    int piece_length;
    unsigned char* pieces; // hashes for pieces
};

struct BencodeTorrent
{
    char announce[MAX_STR_LEN];
    char comment[MAX_STR_LEN];
    int creation_date;
    struct BencodeInfo info;
};

unsigned char info_hash[SHA_DIGEST_LENGTH];

// works when the torrent describes one file (not a dictionary of files)
// for a dictionary of files the `length` object is replaced with a `files` directory object -> (see bep_0003)
void deserialize_bencode_torrent(struct BencodeTorrent* bencode_torrent, char* bencoded_str)
{
    b_get("0.d|announce|", bencoded_str, bencode_torrent->announce); 
    b_get("0.d|comment|", bencoded_str, bencode_torrent->comment); 
    
    char creation_date_str[MAX_STR_LEN];
    b_get("0.d|creation date|", bencoded_str, creation_date_str);
    bencode_torrent->creation_date = atoi(creation_date_str);

    char info_length_str[MAX_STR_LEN];
    b_get("0.d|info|0.d|length|", bencoded_str, info_length_str); 
    bencode_torrent->info.length = atoi(info_length_str);

    b_get("0.d|info|0.d|name|", bencoded_str, bencode_torrent->info.name);

    char info_piece_length_str[MAX_STR_LEN];
    b_get("0.d|info|0.d|piece length|", bencoded_str, info_piece_length_str); 
    bencode_torrent->info.piece_length = atoi(info_piece_length_str);

    int number_of_pieces = bencode_torrent->info.length / bencode_torrent->info.piece_length; // == length of `pieces` object, but needs to calculated before serializing to allocate memory
    bencode_torrent->info.pieces = (char*)malloc(SHA_DIGEST_LENGTH * number_of_pieces);
    b_get("0.d|info|0.d|pieces|", bencoded_str, bencode_torrent->info.pieces);
}

int main(int argc, char *argv[])
{
    char bencoded_str[MAX_STR_LEN] = "d8:announce41:http://bttracker.debian.org:6969/announce7:comment35:'Debian CD from cdimage.debian.org'13:creation datei1573903810e4:infod6:lengthi1e4:name31:debian-10.2.0-amd64-netinst.iso12:piece lengthi1e6:pieces20:0123456789ABCDEF7890ee";

    b_print(bencoded_str);
    printf("\n");

    struct BencodeTorrent bencode_torrent;
    deserialize_bencode_torrent(&bencode_torrent, bencoded_str);

    printf("%.*s\n", SHA_DIGEST_LENGTH * bencode_torrent.info.length / bencode_torrent.info.piece_length, bencode_torrent.info.pieces);

    char dict_str[10000];
    b_get("0.d|info|0.d|", bencoded_str, dict_str);

    SHA1((unsigned char *) dict_str, strlen(dict_str), info_hash);

    for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        printf("%02x", info_hash[i]);
    }
    printf("\n");

    free(bencode_torrent.info.pieces);

    return 0;
}

    

/*
TODO:
*/