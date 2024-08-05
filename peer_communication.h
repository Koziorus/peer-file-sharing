#pragma once

#include "network.h"
#include <openssl/sha.h>

/**
 * @brief Messages returned from a function call indicated by the return value
 * 
 */
extern uchar* msg_peer_handshake[MAX_STR_LEN];

int get_bt_handshake_data(uchar *message, int message_len, uchar *info_hash_out, uchar *peer_id_out);

int peer_handshake(int local_socket, uchar *info_hash, uchar *local_peer_id);
