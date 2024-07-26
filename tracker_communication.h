#pragma once

#include "network.h"
#include "bencode.h"
#include <openssl/sha.h>
#include "helper.h"

#define MAX_PARAM_NUM 10
#define MAX_HEADER_NUM 10
#define MAX_HTTP_RESPONSE_LEN 1000000

#define LOG_VISIBLE

struct TorrentInfo
{
    int length;
    unsigned char name[MAX_BENCODED_OTHER_LEN];
    int piece_length;
    unsigned char* pieces; // hashes for pieces
};

struct TorrentData
{
    unsigned char announce[MAX_BENCODED_OTHER_LEN];
    unsigned char comment[MAX_BENCODED_OTHER_LEN];
    int creation_date;
    struct TorrentInfo info;
};

void deserialize_bencode_torrent(struct TorrentData *torrent_data, unsigned char *bencoded_str);

void get_full_numeric_addr(unsigned char *data, unsigned char *out);

void get_tracker_info(unsigned char *bencoded_torrent, unsigned char *bencoded_response);
