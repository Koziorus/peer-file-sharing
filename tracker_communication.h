#pragma once

#include "network.h"
#include "bencode.h"
#include <openssl/sha.h>
#include <stdarg.h>

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

void get_full_numeric_addr(unsigned char *data, unsigned char *addr_out, unsigned char *port_out);

int http_GET(int peer_socket, char *resource_name, char *params, char *headers, char *response_out);

void create_params(unsigned char *params_out, ...);

void create_headers(unsigned char *headers_out, ...);

int tracker_get_peers(unsigned char *bencoded_torrent, unsigned char info_hash[SHA_DIGEST_LENGTH], unsigned char local_peer_id[SHA_DIGEST_LENGTH], unsigned char peer_addresses[][MAX_STR_LEN], unsigned char peer_ports[][MAX_STR_LEN]);
