#include "network.h"
#include "bencode.h"
#include <openssl/sha.h>
#include <time.h>
#include <regex.h>
#include <byteswap.h>

struct TorrentInfo
{
    int length;
    unsigned char name[MAX_STR_LEN];
    int piece_length;
    unsigned char* pieces; // hashes for pieces
};

struct TorrentData
{
    unsigned char announce[MAX_STR_LEN];
    unsigned char comment[MAX_STR_LEN];
    int creation_date;
    struct TorrentInfo info;
};


#define MAX_PARAM_NUM 10
#define MAX_HEADER_NUM 10
#define MAX_HTTP_RESPONSE_LEN 1000000

#define LOG_VISIBLE

// works when the torrent describes one file (not a dictionary of files)
// for a dictionary of files the `length` object is replaced with a `files` directory object -> (see bep_0003)
void deserialize_bencode_torrent(struct TorrentData* torrent_data, unsigned char* bencoded_str)
{
    b_get("0.d|announce|", bencoded_str, torrent_data->announce); 
    b_get("0.d|comment|", bencoded_str, torrent_data->comment); 
    
    unsigned char creation_date_str[MAX_STR_LEN];
    b_get("0.d|creation date|", bencoded_str, creation_date_str);
    torrent_data->creation_date = atoi(creation_date_str);

    unsigned char info_length_str[MAX_STR_LEN];
    b_get("0.d|info|0.d|length|", bencoded_str, info_length_str); 
    torrent_data->info.length = atoi(info_length_str);

    b_get("0.d|info|0.d|name|", bencoded_str, torrent_data->info.name);

    unsigned char info_piece_length_str[MAX_STR_LEN];
    b_get("0.d|info|0.d|piece length|", bencoded_str, info_piece_length_str); 
    torrent_data->info.piece_length = atoi(info_piece_length_str);

    int number_of_pieces = torrent_data->info.length / torrent_data->info.piece_length; // == length of `pieces` object, but needs to calculated before serializing to allocate memory
    torrent_data->info.pieces = (unsigned char*)malloc(SHA_DIGEST_LENGTH * number_of_pieces);
    b_get("0.d|info|0.d|pieces|", bencoded_str, torrent_data->info.pieces);
}

// `out` should be allocated beforehand
// does not include null terminator
void generate_rand_string(int n, unsigned char* out)
{
    for(int i = 0; i < n; i++)
    {
        out[i] = rand() % 256; // random byte
    }
}

// returns 0 on success, -1 when there's no body
int get_body_from_http(unsigned char* http_data, int http_data_len, unsigned char* body)
{
    // until "\r\n\r\n"
    int i = 0;
    while(!(http_data[i] == '\r' && http_data[i+1] == '\n' && http_data[i+2] == '\r' && http_data[i+3] == '\n'))
    {
        i++;
    }

    i += 4; // jump over the "\r\n\r\n"

    if(i < http_data_len)
    {
        int body_len = http_data_len - i;
        memcpy(body, http_data + i, body_len);
        body[body_len] = '\0';
        return 0;
    }

    return -1;
}

void explicit_http_chars(unsigned char* data, int data_len, unsigned char* out)
{
    // e.g. `ab0f4e` -> "%ab%0f%4e"
    for(int i = 0; i < data_len; i++)
    {
        if(i == 0)
        {
            sprintf(out, "%%%02x", data[i]);
        }
        else
        {
            sprintf(out, "%.*s%%%02x", i*3, out, data[i]); // i* 3 -> for every data[i] there is "%xx"
        }
    }
}

void get_full_numeric_addr(unsigned char* data, unsigned char* out)
{
    uint8_t addr[4];
    int i = 0;
    for(int i = 0; i < 4; i++)
    {
        addr[i] = data[i];
    }

    unsigned char* port_ptr = data + 4;

    uint16_t port = __bswap_16(*((uint16_t*)port_ptr)); // convert from Big Endian to Little Endian
    sprintf(out, "%d.%d.%d.%d:%d", addr[0], addr[1], addr[2], addr[3], port);
}

void get_tracker_info(unsigned char* bencoded_torrent, unsigned char* bencoded_response)
{
    struct TorrentData torrent_data;
    deserialize_bencode_torrent(&torrent_data, bencoded_torrent);
    
    unsigned char info_hash[SHA_DIGEST_LENGTH];

    unsigned char dict_str[MAX_BENCODED_TORRENT_LEN];
    int dict_str_len = b_get("0.d|info|", bencoded_torrent, dict_str);

    SHA1(dict_str, dict_str_len, info_hash);

#ifdef LOG_VISIBLE
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        printf("%02x", info_hash[i]);
    }
    printf("\n");
#endif

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;

    // temporary measure to skip the protocol in URL

    unsigned char remote_domain_name[MAX_STR_LEN];
    int protocol_offset = strlen("http://");
    int port_offset = strcspn(torrent_data.announce + protocol_offset, ":") + protocol_offset + 1;
    int resource_offset = strcspn(torrent_data.announce + port_offset, "/") + port_offset + 1;

    int remote_domain_name_len = strlen(torrent_data.announce) - (port_offset - protocol_offset) + 1; // make space for null terminator
    strncpy(remote_domain_name, torrent_data.announce + protocol_offset, remote_domain_name_len - 1); 
    remote_domain_name[remote_domain_name_len] = '\0';

    unsigned char remote_port[MAX_STR_LEN];
    int remote_port_len = (resource_offset - port_offset); // has space for null terminator
    strncpy(remote_port, torrent_data.announce + port_offset, remote_port_len - 1);
    remote_port[remote_port_len] = '\0';

    unsigned char resource_name[MAX_STR_LEN];
    int resource_name_len = strlen(torrent_data.announce) - resource_offset;
    strncpy(resource_name, torrent_data.announce + resource_offset, resource_name_len);
    resource_name[resource_name_len] = '\0';

    struct addrinfo* remote_addrinfo;
    if(getaddrinfo(remote_domain_name, remote_port, &hints, &remote_addrinfo) != 0)
    {
        failure("getaddrinfo");
    }

    unsigned char remote_host_buf[MAX_STR_LEN];
    unsigned char remote_serv_buf[MAX_STR_LEN];
    if(getnameinfo(remote_addrinfo->ai_addr, remote_addrinfo->ai_addrlen, remote_host_buf, sizeof(remote_host_buf), remote_serv_buf, sizeof(remote_serv_buf), NI_NUMERICHOST) != 0)
    {
        failure("getnameinfo");
    }

    int peer_socket = socket(remote_addrinfo->ai_family, remote_addrinfo->ai_socktype, remote_addrinfo->ai_protocol);
    if(socket == -1)
    {
        failure("socket");
    }

    if(connect(peer_socket, remote_addrinfo->ai_addr, remote_addrinfo->ai_addrlen) == -1)
    {
        failure("connect");
    }

    struct sockaddr local_addr;
    socklen_t local_addr_len = sizeof(local_addr);
    if(getsockname(peer_socket, &local_addr, &local_addr_len) == -1)
    {
        failure("getsockname");
    }

    unsigned char local_host_buf[MAX_STR_LEN];
    unsigned char local_serv_buf[MAX_STR_LEN];
    if(getnameinfo(&local_addr, local_addr_len, local_host_buf, sizeof(local_host_buf), local_serv_buf, sizeof(local_serv_buf), NI_NUMERICHOST | NI_NUMERICSERV) != 0)
    {
        failure("getnameinfo");
    }

#ifdef LOG_VISIBLE
    printf("Connected: %s (%s) -> %s (%s)\n", local_host_buf, local_serv_buf, remote_host_buf, remote_serv_buf);
#endif

    unsigned char param_names[][MAX_STR_LEN] = {
                                        "info_hash",
                                        "peer_id",
                                        "port",
                                        "uploaded",
                                        "downloaded",
                                        "compact",
                                        "left"
                                        };
    unsigned char param_values[MAX_PARAM_NUM][MAX_STR_LEN];
    int param_num = sizeof(param_names) / sizeof(param_names[0]);

    // clear values
    for(int i = 0; i < param_num; i++)
    {
        strcpy(param_values[i], "");
    }

    // info_hash
    unsigned char info_hash_escaped[SHA_DIGEST_LENGTH * 3]; // for each bytes there is a '%' and 2 hex digits
    explicit_http_chars(info_hash, sizeof(info_hash), info_hash_escaped);
    strncpy(param_values[0], info_hash_escaped, sizeof(info_hash_escaped));
    param_values[0][sizeof(info_hash_escaped)] = '\0';

#ifdef LOG_VISIBLE
    printf("%s\n", param_values[0]);
#endif

    // peer_id
    unsigned char peer_id[SHA_DIGEST_LENGTH];
    generate_rand_string(SHA_DIGEST_LENGTH, peer_id);
    unsigned char peer_id_escaped[SHA_DIGEST_LENGTH * 3]; // for each bytes there is a '%' and 2 hex digits
    explicit_http_chars(peer_id, sizeof(peer_id), peer_id_escaped);
    strncpy(param_values[1], peer_id_escaped, sizeof(peer_id_escaped));
    param_values[1][sizeof(peer_id_escaped)] = '\0';

    // port
    strcpy(param_values[2], local_serv_buf);

    // uploaded
    strcpy(param_values[3], "0");

    // downloaded
    strcpy(param_values[4], "0");

    // compact
    strcpy(param_values[5], "1");

    // left
    sprintf(param_values[6], "%d", torrent_data.info.length);

    unsigned char params[MAX_STR_LEN];
    for(int i = 0; i < param_num; i++)
    {
        if(i != 0)
        {
            sprintf(params, "%s&", params);
        }

        sprintf(params, "%s%s=%s", params, param_names[i], param_values[i]);
    }

    unsigned char header_names[][MAX_STR_LEN] = {
                                        "Host"
                                        };
    unsigned char header_values[MAX_HEADER_NUM][MAX_STR_LEN];
    int header_num = sizeof(header_names) / sizeof(header_names[0]);

    // clear values
    for(int i = 0; i < header_num; i++)
    {
        strcpy(header_values[i], "");
    }

    sprintf(header_values[0], "%s:%s", remote_domain_name, remote_port);

    unsigned char headers[MAX_STR_LEN];
    for(int i = 0; i < header_num; i++)
    {
        if(i != 0)
        {
            sprintf(headers, "%s\r\n", params);
        }

        sprintf(headers, "%s%s: %s", headers, header_names[i], header_values[i]);
    }

    unsigned char request[MAX_STR_LEN];
    sprintf(request, "GET /%s?%s HTTP/1.1\r\n%s\r\n\r\n", resource_name, params, headers); // \r\n\r\n is the ending

#ifdef LOG_VISIBLE
    printf("%s\n", request);
#endif

    int bytes_sent = send(peer_socket, request, strlen(request), 0);
    if(bytes_sent < strlen(request) || bytes_sent == -1)
    {
        failure("send");
    }

    // unsigned char response[20000] = "HTTP/1.1 200 OK\r\nServer: mimosa\r\nConnection: Close\r\nContent-Length: 39\r\nContent-Type: text/plain\r\n\r\nd14:failure reason17:torrent not founde";
    // int bytes_received = strlen(response); // exluding null terminator

    unsigned char response[MAX_HTTP_RESPONSE_LEN];
    int bytes_received = recv(peer_socket, response, sizeof(response), 0);
    if(bytes_received == -1)
    {
        failure("recv");
    }
    printf("Received (%d):\n%.*s\n", bytes_received, bytes_received, response);

    unsigned char explicit_response[MAX_HTTP_RESPONSE_LEN * 2]; // potentially two times the size of original response
    explicit_str(response, bytes_received, explicit_response);
    printf("%s\n", explicit_response);

    unsigned char body[MAX_BENCODED_TORRENT_LEN];
    get_body_from_http(response, bytes_received, body);
    b_print(body);

    char peers_addr_data[1000];
    int peers_addr_data_len = b_get("0.d|peers|", body, peers_addr_data);

#ifdef LOG_VISIBLE
    printf("Peers:\n");
    int peers_num = peers_addr_data_len / 6; // 6 bytes per ipv4 and port
    for(int i = 0; i < peers_num; i++)
    {
        char peer_full_addr[100];
        get_full_numeric_addr(peers_addr_data + i * 6, peer_full_addr);
        printf("\t%s\n", peer_full_addr);
    }
#endif

    close(peer_socket);

    free(torrent_data.info.pieces);
}

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

    // b_print(bencoded_torrent);

    get_tracker_info(bencoded_torrent, NULL);

    // unsigned char data[] = {6,9,4,2,4,5};
    // unsigned char out[MAX_STR_LEN];
    // get_full_numeric_addr(data, out);
    // printf("%s", out);

    return 0;
}

    
/*
TODO:
- add checking if HTTP response is "OK" and only then proceed to extract the bencode body
- create a TODO list with tasks to include error/exceptions handling in various parts of the code 
- ! change bencode.c so that it doesnt use checking of '\0' to end loops (except checking for `path`)
- ! check if things are that are sent and received are interpreted with the right endianness
*/