#pragma once

#include "network.h"
#include "bencode.h"
#include <openssl/sha.h>

#define MAX_PARAM_NUM 10
#define MAX_HEADER_NUM 10
#define MAX_HTTP_RESPONSE_LEN 1000000

#define TRACKER_CONNECT_TIMEOUT 20

/**
 * @brief Info dictionary of a bencode torrent
 */
struct TorrentInfo
{
    int length;
    uchar name[MAX_BENCODED_OTHER_LEN];
    int piece_length;
    uchar* pieces; // hashes for pieces
};

/**
 * @brief Whole dictionary of a bencode torrent
 */
struct TorrentData
{
    uchar announce[MAX_BENCODED_OTHER_LEN];
    uchar comment[MAX_BENCODED_OTHER_LEN];
    int creation_date;
    struct TorrentInfo info;
};

/**
 * @brief Deserialize a bencode torrent into TorrentData
 * 
 * @param torrent_data 
 * @param bencoded_str 
 */
void deserialize_bencode_torrent(struct TorrentData *torrent_data, uchar *bencoded_str, int str_len);

/**
 * @brief Get ipv4 address and port from a 6 byte data - address (4 bytes), port (2 bytes)
 * 
 * @param data 
 * @param addr_out 
 * @param port_out 
 */
void get_full_numeric_addr(uchar *data, uchar *addr_out, uchar *port_out);

/**
 * @brief Send a HTTP GET request on an existing connection
 * 
 * @param local_socket 
 * @param resource_name 
 * @param params will be inputted directly into the request
 * @param headers will be inputted directly into the request
 * @param response_out 
 * @param bytes_received_out 
 * @return int;
 * 0 on success;
 * -1 on error (errno)
 */
int http_GET(int local_socket, char *resource_name, char *params, char *headers, char *response_out, int *bytes_received_out);

/**
 * @brief Create params for http_GET; last paramater should be NULL
 * 
 * @param params_out 
 * @param ... param_name0, param_value0, ..., param_nameN, param_valueN, NULL 
 */
void create_params(uchar *params_out, ...);

/**
 * @brief Create headers for http_GET; last paramater should be NULL
 * 
 * @param headers_out 
 * @param ... header_name0, header_value0, ..., header_nameN, header_valueN, NULL 
 */
void create_headers(uchar *headers_out, ...);


/**
 * @brief Connect to a tracker given a torrent
 * 
 * @param bencoded_torrent 
 * @param info_hash 
 * @param local_peer_id 
 * @param peer_addresses_out 
 * @param peer_ports_out 
 * @param peers_num_out 
 * @return int;
 * 0 on success;
 * -1 on error (errno)
 */
int tracker_get_peers(uchar *bencode_torrent, int bencode_str_len, uchar info_hash[SHA_DIGEST_LENGTH], uchar local_peer_id[SHA_DIGEST_LENGTH], uchar peer_addresses_out[][MAX_STR_LEN], uchar peer_ports_out[][MAX_STR_LEN], int *peers_num_out);
