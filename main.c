#include <time.h>
#include "helper.h"
#include "tracker_communication.h"

#define CONNECTION_REFUSED 111
#define CONNECTION_RESET_BY_PEER 104

// TEMP
#define MAX_TRACKER_ADDRESS_NUM 100

/**
 * @brief Messages returned from a function call indicated by the return value
 * 
 */
uchar* msg_peer_handshake[MAX_STR_LEN] = 
{
    "Matching",
    "Not matching",
    "Wrong message format",
    "Response timeout"
};

uchar* msg_start_TCP_connection[MAX_STR_LEN] =
{
    "Connected",
    "Timed out"
};

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
        uchar* explicit_response[MAX_STR_LEN];
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

void print_nested(int nesting, const char* format, ...)
{
    va_list args;
    va_start(args, nesting);
    for(int i = 0; i < nesting; i++)
    {
        printf("\t");
    }

    vprintf(format, args);

    va_end(args);
}

void download_torrent(uchar* bencode_torrent, int bencode_torrent_len)
{
    // generate info_hash
    uchar info_str[MAX_BENCODED_TORRENT_LEN];
    int dict_str_len = b_get("0.d|info|", bencode_torrent, bencode_torrent_len, info_str);
    uchar info_hash[SHA_DIGEST_LENGTH];
    SHA1(info_str, dict_str_len, info_hash);

    // create a peer_id
    uchar local_peer_id[SHA_DIGEST_LENGTH];
    generate_rand_str(SHA_DIGEST_LENGTH, local_peer_id);

    uchar peer_addrs[MAX_TRACKER_ADDRESS_NUM][MAX_STR_LEN];
    uchar peer_ports[MAX_TRACKER_ADDRESS_NUM][MAX_STR_LEN];
    int peers_num;
    if(tracker_get_peers(bencode_torrent, bencode_torrent_len, info_hash, local_peer_id, peer_addrs, peer_ports, &peers_num) == -1)
    {
        failure("tracker_get_peers");
    }
        
    for(int i = 0; i < peers_num; i++)
    {
        int print_nesting = 1;
        printf("%s:%s\n", peer_addrs[i], peer_ports[i]);

        struct timeval peer_timeout;
        peer_timeout.tv_sec = 5;
        peer_timeout.tv_usec = 0;

        uchar local_addr[MAX_STR_LEN];
        uchar local_port[MAX_STR_LEN];
        int local_socket;
        int conn_ret = start_TCP_connection(peer_addrs[i], peer_ports[i], local_addr, local_port, &local_socket, &peer_timeout);
        if(conn_ret == -1)
        {
            if(errno == CONNECTION_REFUSED)
            {
                print_nested(print_nesting, "Connection refused\n");
                continue;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            print_nested(print_nesting, "%s\n", msg_start_TCP_connection[conn_ret]);
            if(conn_ret == 1)
            {
                continue;
            }
        }

        int handshk_ret = peer_handshake(local_socket, info_hash, local_peer_id);
        if(handshk_ret == -1)
        {
            if(errno == CONNECTION_RESET_BY_PEER)
            {
                print_nested(print_nesting, "Connection reset by peer\n");
            }
            else
            {
                failure("peer_handshake");
            }
        }
        else
        {
            print_nested(print_nesting, "%s\n", msg_peer_handshake[handshk_ret]);
        }

        close(local_socket);
    }
}

/**
 * @brief main function
 * 
 * @param argc not in use
 * @param argv not in use
 * @return int 
 */
int main(int argc, uchar *argv[])
{
    srand(time(NULL)); // for random peer_id generation

    FILE* torrent_file;
    torrent_file = fopen("debian-12.6.0-i386-netinst.iso.torrent", "r");
    
    if(torrent_file == NULL)
    {
        failure("open");
    }
    
    uchar bencode_torrent[MAX_BENCODED_TORRENT_LEN];
    int bencode_torrent_len = fread(bencode_torrent, sizeof(uchar), MAX_BENCODED_TORRENT_LEN, torrent_file);
    fclose(torrent_file);

    printf("%s\n", bencode_torrent);
    b_print(bencode_torrent, bencode_torrent_len);

    download_torrent(bencode_torrent, bencode_torrent_len);

    return 0;
}
 
// TODO general
// - add checking if HTTP response is "OK" and only then proceed to extract the bencode body
// - add documentation for all functions
// - add asserts
// - change type of *_len to size_t
// - update doxygen function briefs
// - add logging to a file (and then open the log file in real time)