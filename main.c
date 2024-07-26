#include <time.h>
#include "helper.h"
#include "tracker_communication.h"

int main(int argc, unsigned char *argv[])
{
    srand(time(NULL)); // for random peer_id generation

    FILE* torrent_file;
    torrent_file = fopen("debian-12.6.0-i386-netinst.iso.torrent", "r");
    
    if(torrent_file == NULL)
    {
        failure("open");
    }
    
    unsigned char bencoded_torrent[MAX_BENCODED_TORRENT_LEN];
    fread(bencoded_torrent, sizeof(unsigned char), MAX_BENCODED_TORRENT_LEN, torrent_file);
    fclose(torrent_file);

    printf("%s\n", bencoded_torrent);
    b_print(bencoded_torrent);

    // generate info_hash
    unsigned char info_str[MAX_BENCODED_TORRENT_LEN];
    int dict_str_len = b_get("0.d|info|", bencoded_torrent, info_str);
    unsigned char info_hash[SHA_DIGEST_LENGTH];
    SHA1(info_str, dict_str_len, info_hash);

    // create a peer_id
    unsigned char local_peer_id[SHA_DIGEST_LENGTH];
    generate_rand_str(SHA_DIGEST_LENGTH, local_peer_id);

    unsigned char peer_addrs[100][MAX_STR_LEN];
    unsigned char peer_ports[100][MAX_STR_LEN];
    int peers_num = tracker_get_peers(bencoded_torrent, info_hash, local_peer_id, peer_addrs, peer_ports);

    for(int i = 0; i < peers_num; i++)
    {
        printf("%s:%s\n", peer_addrs[i], peer_ports[i]);

        unsigned char local_addr[MAX_STR_LEN];
        unsigned char local_port[MAX_STR_LEN];
        int local_socket = start_TCP_connection(peer_addrs[i], peer_ports[i], local_addr, local_port);

        //message = \x13BitTorrent protocol\x00\x00\x00\x00\x00\x00\x00\x00`info_hash``local_peer_id`
        unsigned char message[MAX_STR_LEN];
        unsigned char* message_end_ptr = message; 
        strcpy(message_end_ptr, "\x13""BitTorrent protocol");
        message_end_ptr += strlen(message);
        memset(message_end_ptr, 0, 8); // 8 reserved bytes
        message_end_ptr += 8;
        memcpy(message_end_ptr, info_hash, sizeof(info_hash));
        message_end_ptr += sizeof(info_hash);
        memcpy(message_end_ptr, local_peer_id, sizeof(local_peer_id));
        message_end_ptr += sizeof(local_peer_id);

        int message_size = message_end_ptr - message;
        
        int bytes_sent = send(local_socket, message, message_size, 0);
        if(bytes_sent == -1)
        {
            failure("send");
        }

        unsigned char response[MAX_STR_LEN];
        int bytes_received = recv(local_socket, response, sizeof(response), 0);
        printf("Received (%d bytes): %.*s\n", bytes_received, bytes_received, response);

        close(local_socket);
    }

    return 0;
}
    
/*
TODO:
- add checking if HTTP response is "OK" and only then proceed to extract the bencode body
- create a TODO list with tasks to include error/exceptions handling in various parts of the code 
- ! change bencode.c so that it doesnt use checking of '\0' to end loops (except checking for `path`)
*/