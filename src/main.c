#include <time.h>
#include "helper.h"
#include "tracker_communication.h"
#include "peer_communication.h"

#define CONNECTION_REFUSED 111
#define CONNECTION_RESET_BY_PEER 104

// TEMP
#define MAX_TRACKER_ADDRESS_NUM 100

// WIP
int download_torrent(uchar* bencode_torrent, int bencode_torrent_len)
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
 * @param argc 
 * @param argv
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
// - update doxygen function briefs
// - add logging to a file (and then open the log file in real time)
// - divide functions into smaller ones
// - ? explicitly include libraries that are used in a file (dont assume thery are used nested deeper in some other library)