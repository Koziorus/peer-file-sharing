#include "peer_communication.h"

int get_bt_handshake_data(uchar* message, int message_len, uchar* info_hash_out, uchar* peer_id_out)
{
    // message: protocol_id_len; protocol_id; extensions; info_hash; peer_id
    uchar protocol_id[] = {'B','i','t','T','o','r','r','e','n','t',' ','p','r','o','t','o','c','o','l'};
    uchar protocol_id_len[] = {sizeof(protocol_id)};
    uchar extensions[] = {0, 0, 0, 0, 0, 0, 0, 0}; // no extensions of bittorrent protocol
    int info_hash_size = SHA_DIGEST_LENGTH;
    int peer_id_size = SHA_DIGEST_LENGTH;

    // if message length or protocol identifier is incorrent    
    if(!(message_len == sizeof(protocol_id_len) + sizeof(protocol_id) + sizeof(extensions) + info_hash_size + peer_id_size 
    && memcmp(message, protocol_id_len, sizeof(protocol_id_len)) == 0 
    && memcmp(message + sizeof(protocol_id_len), protocol_id, sizeof(protocol_id)) == 0))
    {
        return 1;
    }

    int offset = sizeof(protocol_id_len) + sizeof(protocol_id) + sizeof(extensions);
    memcpy(info_hash_out, message + offset, info_hash_size);
    offset += info_hash_size;
    memcpy(peer_id_out, message + offset, peer_id_size);

    return 0;
}

uchar* msg_peer_handshake[MAX_STR_LEN] = 
{
    "Matching",
    "Not matching",
    "Wrong message format",
    "Response timeout"
};

/**
 * @brief 
 * 
 * @param local_socket 
 * @param info_hash 
 * @param local_peer_id 
 * @return int;
 * 0 on matching `info_hash`;
 * 1 on not matching `info_hash`;
 * 2 wrong message format;
 * 3 on response timeout 
 */
int peer_handshake(int local_socket, uchar* info_hash, uchar* local_peer_id)
{
    int info_hash_size = SHA_DIGEST_LENGTH;
    int peer_id_size = SHA_DIGEST_LENGTH;

    //message = \x13BitTorrent protocol\x00\x00\x00\x00\x00\x00\x00\x00`info_hash``local_peer_id`
    uchar message[MAX_STR_LEN];
    uchar* message_end_ptr = message; 
    strcpy(message_end_ptr, "\x13""BitTorrent protocol");
    message_end_ptr += strlen(message);
    memset(message_end_ptr, 0, 8); // extensions (none is set)
    message_end_ptr += 8;
    memcpy(message_end_ptr, info_hash, info_hash_size);
    message_end_ptr += info_hash_size;
    memcpy(message_end_ptr, local_peer_id, peer_id_size);
    message_end_ptr += peer_id_size;

    int message_size = message_end_ptr - message;
    
    int bytes_sent = send(local_socket, message, message_size, 0);
    if(bytes_sent == -1 || bytes_sent < message_size)
    {
        return -1;
    }

    LOG(printf("Sending message...\n");)

    fd_set set;
    FD_ZERO(&set);
    FD_SET(local_socket, &set);
    int max_socket = local_socket;

    fd_set reads = set;

    struct timeval timeout; // timeout = 1s
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if(select(max_socket + 1, &reads, 0, 0, &timeout) == -1)
    {
        return -1;
    }

    if(FD_ISSET(local_socket, &reads))
    {
        uchar response[MAX_STR_LEN];
        int bytes_received = recv(local_socket, response, sizeof(response), 0);
        if(bytes_received == -1)
        {
            return -1;
        }
        uchar explicit_response[MAX_STR_LEN];
        int explicit_response_len = explicit_str(response, bytes_received, explicit_response);
        LOG(printf("Received (%d bytes): %.*s\n", bytes_received, explicit_response_len, explicit_response);)

        uchar info_hash_recv[SHA_DIGEST_LENGTH];
        uchar peer_id_recv[SHA_DIGEST_LENGTH];
        if(get_bt_handshake_data(response, bytes_received, info_hash_recv, peer_id_recv) == 0)
        {
            // if info hashes match
            if(memcmp(info_hash_recv, info_hash, SHA_DIGEST_LENGTH) == 0)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }
        else // get_bt_handshake_data == 1
        {
            return 2;
        }
    }
    else
    {
        return 3;
    }
}