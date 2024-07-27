#include "tracker_communication.h"

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

void get_full_numeric_addr(unsigned char* data, unsigned char* addr_out, unsigned char* port_out)
{
    uint8_t addr[4];
    int i = 0;
    for(int i = 0; i < 4; i++)
    {
        addr[i] = data[i];
    }

    unsigned char* port_ptr = data + 4;

    uint16_t port = __bswap_16(*((uint16_t*)port_ptr)); // convert from Big Endian to Little Endian
    sprintf(addr_out, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
    sprintf(port_out, "%d", port);
}

int http_GET(int local_socket, char* resource_name, char* params, char* headers, char* response_out)
{
    unsigned char request[MAX_STR_LEN];
    sprintf(request, "GET /%s?%s HTTP/1.1\r\n%s\r\n\r\n", resource_name, params, headers); // \r\n\r\n is the ending

#ifdef LOG_VISIBLE
    printf("%s\n", request);
#endif

    int bytes_sent = send(local_socket, request, strlen(request), 0);
    if(bytes_sent < strlen(request) || bytes_sent == -1)
    {
        failure("send");
    }

    unsigned char response[MAX_HTTP_RESPONSE_LEN];
    int bytes_received = recv(local_socket, response, sizeof(response), 0);
    if(bytes_received == -1)
    {
        failure("recv");
    }

#ifdef LOG_VISIBLE
    printf("Received (%d bytes):\n%.*s\n", bytes_received, bytes_received, response);
#endif
    
    // unsigned char explicit_response[MAX_HTTP_RESPONSE_LEN * 2]; // potentially two times the size of original response
    // explicit_str(response, bytes_received, explicit_response);
    // printf("%s\n", explicit_response);

    strncpy(response_out, response, bytes_received);
    return bytes_received;
}

// pass NULL as the last argument
void create_params(unsigned char* params_out, ...)
{
    va_list arg_list;
    va_start(arg_list, params_out);

    for(int i = 0; TRUE; i++)
    {
        unsigned char* param_name = va_arg(arg_list, unsigned char*);
        if(param_name == NULL) //check if all arguments have been exhausted from arg_list
        {
            va_end(arg_list);
            break;
        }

        unsigned char* param_value = va_arg(arg_list, unsigned char*);

        if(i != 0)
        {
            sprintf(params_out, "%s&", params_out);
        }
        
        sprintf(params_out, "%s%s=%s", params_out, param_name, param_value);
    }
}

// pass NULL as the last argument
void create_headers(unsigned char* headers_out, ...)
{
    va_list arg_list;
    va_start(arg_list, headers_out);

    for(int i = 0; TRUE; i++)
    {
        unsigned char* header_name = va_arg(arg_list, unsigned char*);
        if(header_name == NULL) //check if all arguments have been exhausted from arg_list
        {
            va_end(arg_list);
            break;
        }

        unsigned char* header_value = va_arg(arg_list, unsigned char*);

        if(i != 0)
        {
            sprintf(headers_out, "%s\r\n", headers_out);
        }

        sprintf(headers_out, "%s%s: %s", headers_out, header_name, header_value);
    }
}

// connects to a tracker and gets peer addresses
int tracker_get_peers(unsigned char* bencoded_torrent, unsigned char info_hash[SHA_DIGEST_LENGTH], unsigned char local_peer_id[SHA_DIGEST_LENGTH], unsigned char peer_addresses[][MAX_STR_LEN], unsigned char peer_ports[][MAX_STR_LEN])
{
    struct TorrentData torrent_data;
    deserialize_bencode_torrent(&torrent_data, bencoded_torrent);
    
#ifdef LOG_VISIBLE
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        printf("%02x", info_hash[i]);
    }
    printf("\n");
#endif

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

    char local_addr[MAX_STR_LEN];
    char local_port[MAX_STR_LEN];
    int local_socket;
    if(start_TCP_connection(remote_domain_name, remote_port, local_addr, local_port, &local_socket) == -1)
    {
        return -1;
    }

    unsigned char info_hash_escaped[SHA_DIGEST_LENGTH * 3 + 1]; // for each bytes there is a '%' and 2 hex digits, at the end a null terminator
    http_explicit_hex(info_hash, SHA_DIGEST_LENGTH, info_hash_escaped);
    info_hash_escaped[sizeof(info_hash_escaped)] = '\0';

    unsigned char peer_id_escaped[SHA_DIGEST_LENGTH * 3]; // for each bytes there is a '%' and 2 hex digits, at the end a null terminator
    http_explicit_hex(local_peer_id, SHA_DIGEST_LENGTH, peer_id_escaped);
    peer_id_escaped[sizeof(peer_id_escaped)] = '\0';

    unsigned char data_left[MAX_STR_LEN];
    sprintf(data_left, "%d", torrent_data.info.length);

    unsigned char params[MAX_STR_LEN];
    create_params(params,   "info_hash", info_hash_escaped, 
                            "peer_id", peer_id_escaped,
                            "port", local_port,
                            "uploaded", "0",
                            "downloaded", "0",
                            "compact", "1",
                            "left", data_left,
                            NULL);

    unsigned char host[MAX_STR_LEN];
    sprintf(host, "%s:%s", remote_domain_name, remote_port);

    unsigned char headers[MAX_STR_LEN];
    create_headers(headers, "Host", host,
                            NULL);

    char response[MAX_HTTP_RESPONSE_LEN];
    int bytes_received = http_GET(local_socket, resource_name, params, headers, response);
    
    close(local_socket);

    unsigned char body[MAX_BENCODED_TORRENT_LEN];
    http_response_extract_body(response, bytes_received, body);
    b_print(body);

    char peers_addr_data[MAX_BENCODED_OTHER_LEN];
    int peers_addr_data_len = b_get("0.d|peers|", body, peers_addr_data);

    int peers_num = peers_addr_data_len / 6; // 6 bytes per ipv4 and port
    for(int i = 0; i < peers_num; i++)
    {
        unsigned char peer_addr[MAX_STR_LEN];
        unsigned char peer_port[MAX_STR_LEN];
        get_full_numeric_addr(peers_addr_data + i * 6, peer_addr, peer_port);

        strcpy(peer_addresses[i], peer_addr);
        strcpy(peer_ports[i], peer_port);
    }

    free(torrent_data.info.pieces);

    return peers_num;
}

/*
TODO:
- moze byc cos nie tak z przetwarzaniem/odbieraniem danych od trackera (czesc danych jest zerowa)
*/