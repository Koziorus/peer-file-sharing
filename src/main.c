#include <time.h>
#include "helper.h"
#include "tracker_communication.h"
#include "peer_communication.h"
#include <sys/queue.h>

#define CONNECTION_REFUSED 111
#define CONNECTION_RESET_BY_PEER 104

// TEMP
#define MAX_TRACKER_ADDRESS_NUM 100

#define MAX_MESSAGE_LEN 100000

#define MESSAGE_LENGTH_SIZE 4

// 2^14 (16KiB)
#define BLOCK_SIZE 1<<14

struct ResourceBlock
{
    int size;
    int piece_index;
    int piece_offset;
    uchar* data_ptr;
};

struct ResourceQueueNode
{
    struct ResourceBlock resource_block;
    struct ResourceQueueNode* next;
};

struct ResourceQueue
{
    struct ResourceQueueNode* front;
    struct ResourceQueueNode* back;
};

// // inserts into the front
// void resource_queue_insert(struct ResourceQueue resource_queue, struct ResourceBlock resource_block)
// {
//     struct ResourceQueueNode* new_queue_node = (struct ResourceQueueNode*)malloc(sizeof(struct ResourceQueueNode));
//     new_queue_node->next = resource_queue.front;
//     new_queue_node->resource_block = resource_block;

//     resource_queue.front = new_queue_node;
// }

void resource_queue_push(struct ResourceQueue* resource_queue, struct ResourceBlock resource_block)
{
    struct ResourceQueueNode* new_queue_node = (struct ResourceQueueNode*)malloc(sizeof(struct ResourceQueueNode));
    new_queue_node->next = NULL;
    new_queue_node->resource_block = resource_block;

    if((*resource_queue).back != NULL)
    {
        (*resource_queue).back->next = new_queue_node;
    }

    (*resource_queue).back = new_queue_node;
    if((*resource_queue).front == NULL)
    {
        (*resource_queue).front = new_queue_node;
    }
}

int resource_queue_pop(struct ResourceQueue* resource_queue, struct ResourceBlock* resource_out)
{
    if((*resource_queue).front == NULL)
    {
        return -1;
    }

    struct ResourceQueueNode* next_queue_node = (*resource_queue).front->next;
    free((*resource_queue).front);
    (*resource_queue).front = next_queue_node;

    if((*resource_queue).front == NULL)
    {
        (*resource_queue).back = NULL;
    }
    
    return 0;
}

void create_work(uchar* pieces_mem, struct ResourceQueue* resource_queue_out, int piece_size, int block_size, int piece_num)
{
    int file_size = (piece_num * piece_size); 
    int block_num = file_size/ block_size;
    int stray_block_size = file_size - block_num * block_size; // size of the last block ([0;block_size-1])

    int i;
    for(i = 0; i < block_num; i++)
    {
        struct ResourceBlock resource_block;
        resource_block.size = BLOCK_SIZE;
        resource_block.piece_offset = i * resource_block.size;
        resource_block.data_ptr = pieces_mem + resource_block.piece_offset;
        resource_block.piece_index = resource_block.piece_offset / piece_size; // integer division
        resource_queue_push(resource_queue_out, resource_block);
    }

    if(stray_block_size > 0)
    {
        struct ResourceBlock resource_block;
        resource_block.size = stray_block_size;
        resource_block.piece_offset = i * BLOCK_SIZE;
        resource_block.data_ptr = pieces_mem + resource_block.piece_offset;
        resource_block.piece_index = resource_block.piece_offset / piece_size; // integer division
        resource_queue_push(resource_queue_out, resource_block);
    }
}

void resource_queue_create(struct ResourceQueue* resource_queue)
{
    (*resource_queue).front = NULL;
    (*resource_queue).back = NULL;
}

void resource_queue_free(struct ResourceQueue resource_queue)
{
    struct ResourceQueueNode* queue_node = resource_queue.front;
    while(queue_node != NULL)
    {
        struct ResourceQueueNode* next_queue_node = queue_node->next;
        free(queue_node);
        queue_node = next_queue_node;
    } 
}

uint32_t be32_swap_le32(uint32_t x)
{
    return __bswap_32(x);
}

#ifndef __INTELLISENSE__ // suppressing an intellisense bug with C23 feature
typedef enum : uchar
{
    CHOKE = 0,
    UNCHOKE = 1,
    INTERESTED = 2,
    NOT_INTERESTED = 3,
    HAVE = 4,
    BITFIELD = 5,
    REQUEST = 6,
    PIECE = 7,
    CANCEL = 8
} MessageType;
#else
// only above enum will really be compiled and used. The enum below is used only to not cause intellisense error squiggles of not defined enum type
typedef enum
{
    CHOKE = 0,
    UNCHOKE = 1,
    INTERESTED = 2,
    NOT_INTERESTED = 3,
    HAVE = 4,
    BITFIELD = 5,
    REQUEST = 6,
    PIECE = 7,
    CANCEL = 8
} MessageType;
#endif

uchar* MessageType_STR[MAX_STR_LEN] =
{
    "CHOKE",
    "UNCHOKE",
    "INTERESTED",
    "NOT_INTERESTED",
    "HAVE",
    "BITFIELD",
    "REQUEST",
    "PIECE",
    "CANCEL"
};

// TODO: create asserts that assert the correct size of fields (e.g. in the struct Message)

/**
 * @brief Message send/received between peers
 * all the fields in this struct should be appropriate size (except the enum which is cast to uchar when used)
 */
struct Message
{
    uint32_t length; // (LE) size of the payload (X bytes) and id (1 byte)
    MessageType id;
    uchar* payload;
};

/**
 * @brief 
 * 
 * @param local_socket 
 * @param length 
 * @param message_type 
 * @param payload_out 
 * @return int;
 * `0` on success;
 * `1` on keep-alive message (no id, no payload);
 * `-1` on error (errno) 
 */
int recv_message(int local_socket, struct Message* message_out)
{
    uchar message[MAX_MESSAGE_LEN];
    int bytes_received = recv(local_socket, message, sizeof(message), 0);
    if(bytes_received == -1)
    {
        return -1;
    }

    uint32_t length_be = *((uint32_t*)message);    
    uint32_t length = be32_swap_le32(length_be);
    message_out->length = length;

    if(length == 0)
    {
        message_out->id = 0;
        message_out->payload = NULL;
        return 1; 
    }

    message_out->id = *(message + sizeof(length_be));

    int payload_size = length - 1;
    message_out->payload = (uchar*)malloc(payload_size);
    memcpy(message_out->payload, message + sizeof(length_be) + sizeof(message_out->id), payload_size);    
    
    return 0;
}

/**
 * @brief 
 * 
 * @param local_socket 
 * @param message 
 * @return int;
 * `0` on success;
 * `1` on not sending the whole message (TEMP)
 * `-1` on error (errno)
 */
int send_message(int local_socket, struct Message message)
{
    // TODO: check if the entire message was sent in one go, if not then try sending the rest until it succeeds

    int message_data_size = sizeof(uint32_t) + message.length;
    uchar* message_data = (uchar*)malloc(message_data_size);
    
    uint32_t length_be = be32_swap_le32(message.length);
    memcpy(message_data, &length_be, sizeof(length_be));

    memcpy(message_data + sizeof(length_be), &(message.id), 1);
    memcpy(message_data + sizeof(length_be) + 1, message.payload, (int)message.length - 1);

    int bytes_sent = send(local_socket, message_data, message_data_size, 0);
    if(bytes_sent == -1)
    {
        return -1;
    }
    else if(bytes_sent < message_data_size)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief Get bit in an array of 1 bytes (e.g. 0th bit is the most significant bit in the first byte)
 * 
 * @param index 
 * @param data 
 * @return uchar 
 */
uchar get_bit_from_data(int index, uchar* data)
{
    int byte_id = index / 8;
    int bit_id = (7 - (index - byte_id*8));
    unsigned int temp = data[byte_id] & (1 << bit_id);
    return temp >> bit_id;
}

// 'A bitfield of the wrong length is considered an error. Clients should drop the connection if they receive bitfields that are not of the correct size, or if the bitfield has any of the spare bits set.'
/**
 * @brief 
 * 
 * @param bitfield 
 * @param bitfield_len 
 * @param torrent_size 
 * @return int;
 * `0` on correct bitfield size;
 * `1` on incorrect bitfield size 
 */
int check_bitfield_size_match_torrent_size(uchar* bitfield, int bitfield_len, int torrent_size)
{
    // 8 blocks per byte
    int bitfield_size = bitfield_len * 8 * BLOCK_SIZE;

    // look for the first active bit from the end (here it's from LSB)
    uchar last_block = bitfield[bitfield_len - 1];
    int last_block_potential_size = 1; // assumes that the bitfield byte has at least one block (1 bit in use)

    // check 7 bits
    for(int i = 0; i < 7; i++)
    {
        unsigned int bit_value = (last_block & (1 << i)) >> i;
        if(bit_value == 1)
        {
            last_block_potential_size = 8 - i;
            break;
        }
    }

    if(bitfield_size - last_block_potential_size > torrent_size)
    {
         return 1; 
    }
    else if(bitfield_size - last_block_potential_size <= torrent_size)
    {
        if((torrent_size - (bitfield_size - last_block_potential_size) <= 8 - last_block_potential_size))
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }   
}

/**
 * @brief 
 * 
 * @param bitfield_message 
 * @param resource_queue 
 * @param resource_block_out 
 * @return int;
 * `0` on success;
 * `1` on no available piece for the whole resource queue
 */
int search_for_resource_for_peer(struct Message bitfield_message, struct ResourceQueue resource_queue, struct ResourceBlock* resource_block_out)
{
    struct ResourceBlock resource_block;
    // TODO: continue to the next peer when the whole queue has been searched (check for the address of data_ptr)
    uchar* first_resource_ptr = resource_queue.front->resource_block.data_ptr; 
    
    uchar is_piece_available = 0;
    for(int i = 0; is_piece_available == 0; i++)
    {
        if(i != 0 && resource_block.data_ptr == first_resource_ptr)
        {
            return 1;
        }

        resource_queue_pop(&resource_queue, &resource_block);
        is_piece_available = get_bit_from_data(resource_block.piece_index, bitfield_message.payload);

        if(is_piece_available == 0)
        {
            resource_queue_push(&resource_queue, resource_block);
        }
    }

    *resource_block_out = resource_block;

    return 0;
}

int download_piece(int local_socket, struct ResourceBlock resource_block)
{
    struct Message request_message;
    request_message.id = REQUEST;

    // payload
    uint32_t request_index_be = be32_swap_le32(resource_block.piece_index);
    uint32_t request_begin_be = be32_swap_le32(resource_block.piece_offset);
    uint32_t request_length_be = be32_swap_le32(resource_block.size);

    int request_payload_size = sizeof(request_index_be) + sizeof(request_begin_be) + sizeof(request_length_be);
    request_message.payload = (uchar*)malloc(request_payload_size); // MEM

    // message length
    request_message.length = sizeof(request_message.id) + request_payload_size;

    // message length (BE)
    uint32_t request_message_length_be = be32_swap_le32(request_message.length);

    uchar* req_payload_part_ptr = request_message.payload;
    memcpy(req_payload_part_ptr, &request_index_be, sizeof(request_index_be));
    req_payload_part_ptr += sizeof(request_index_be);
    memcpy(req_payload_part_ptr, &request_begin_be, sizeof(request_begin_be));
    req_payload_part_ptr += request_begin_be;
    memcpy(req_payload_part_ptr, &request_length_be, sizeof(request_length_be));

    if(send_message(local_socket, request_message) != 0)
    {
        return 1; // didnt send the whole message or error
    }

    free(request_message.payload);

    struct Message piece_message;
    if(recv_message(local_socket, &piece_message) != 0)
    {
        return 2; // keep alive or error
    }

    if(piece_message.id != PIECE)
    {
        return 3; // didn't receive a piece message
    }

    int recv_block_size = (int)piece_message.length - sizeof(piece_message.id);
    if(recv_block_size != BLOCK_SIZE)
    {
        return 4; // received a block with a size that does not match size from request message
    }

    uint32_t recv_piece_index_be;
    uint32_t recv_piece_offset_be;

    uchar* recv_piece_payload_part_ptr = piece_message.payload;
    memcpy(&recv_piece_index_be, recv_piece_payload_part_ptr, sizeof(recv_piece_index_be));
    recv_piece_payload_part_ptr += sizeof(recv_piece_index_be);
    memcpy(&recv_piece_offset_be, recv_piece_payload_part_ptr, sizeof(recv_piece_offset_be));
    recv_piece_payload_part_ptr += sizeof(recv_piece_offset_be);
    memcpy(resource_block.data_ptr, recv_piece_payload_part_ptr, recv_block_size);

    return 0;
}

// WIP
int download_file(uchar* bencode_torrent, int bencode_torrent_len)
{
    // generate info_hash
    uchar info_data[MAX_BENCODED_TORRENT_LEN];
    int dict_str_len = b_get("0.d|info|", bencode_torrent, bencode_torrent_len, info_data, 0);
    uchar info_hash[SHA_DIGEST_LENGTH];
    SHA1(info_data, dict_str_len, info_hash);

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

    // duplicate - FIX -> tracker_get_peers
    struct TorrentData torrent_data;
    deserialize_bencode_torrent(&torrent_data, bencode_torrent, bencode_torrent_len);

    int pieces_mem_size = torrent_data.info.length;
    uchar* pieces_mem = (uchar*)malloc(pieces_mem_size); // MEM
    int piece_num = torrent_data.info.length / torrent_data.info.piece_length;

    struct ResourceQueue resource_queue;
    resource_queue_create(&resource_queue); 
    create_work(pieces_mem, &resource_queue, torrent_data.info.piece_length, BLOCK_SIZE, piece_num);

    for(int i = 0; i < peers_num; i++)
    {
        int nesting = 1;
        printf("%s:%s\n", peer_addrs[i], peer_ports[i]);

        struct timeval peer_timeout;
        peer_timeout.tv_sec = 1;
        peer_timeout.tv_usec = 0;

        uchar local_addr[MAX_STR_LEN];
        uchar local_port[MAX_STR_LEN];
        int local_socket;
        int conn_ret = start_TCP_connection(peer_addrs[i], peer_ports[i], local_addr, local_port, &local_socket, &peer_timeout);
        if(conn_ret == -1)
        {
            if(errno == CONNECTION_REFUSED)
            {
                print_nested(nesting, "Connection refused\n");
                continue;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            print_nested(nesting, "%s\n", msg_start_TCP_connection[conn_ret]);
            if(conn_ret == 1)
            {
                continue;
            }
        }

        int handshake_ret = peer_handshake(local_socket, info_hash, local_peer_id);
        if(handshake_ret == -1)
        {
            if(errno == CONNECTION_RESET_BY_PEER)
            {
                print_nested(nesting, "Connection reset by peer\n");
            }
            else
            {
                failure("peer_handshake");
            }
        }
        else
        {
            print_nested(nesting, "%s\n", msg_peer_handshake[handshake_ret]);

            if(handshake_ret != 0)
            {
                continue; 
            }
        }

        struct Message bitfield_message;
        if(recv_message(local_socket, &bitfield_message) == -1)
        {
            return -1;
        }

        // expects a bitfield as a first message
        if(bitfield_message.id != BITFIELD)
        {
            return 1; // didn't receive a bitfield message
        }

        int bitfield_len = bitfield_message.length - sizeof(bitfield_message.id);
        if(check_bitfield_size_match_torrent_size(bitfield_message.payload, bitfield_len, pieces_mem_size) != 0)
        {
            print_nested(nesting, "%dB\n", bitfield_len * 8 * BLOCK_SIZE);
            continue; // TEMP
            // return 2; // bitfield size and torrent size are not matched
        }

        print_nested(nesting, "Message: %s\n", MessageType_STR[bitfield_message.id]);

        print_nested(nesting, "Searching for right resource for peer\n");\

        struct ResourceBlock resource_block;

        while(search_for_resource_for_peer(bitfield_message, resource_queue, &resource_block) == 0)
        {
            print_nested(nesting, "Piece available\n");

            struct Message message;
            for(int i = 0; message.id != UNCHOKE; i++)
            {
                if(recv_message(local_socket, &message) == -1)
                {
                    return -1;
                }
                print_nested(nesting, "Message: %s\n", MessageType_STR[message.id]);
            }

            download_piece(local_socket, resource_block);
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

    FILE* torrent_file = fopen("debian-12.6.0-amd64-netinst.iso.torrent", "r");
    if(torrent_file == NULL)
    {
        failure("open");
    }
    
    uchar bencode_torrent[MAX_BENCODED_TORRENT_LEN];
    int bencode_torrent_len = fread(bencode_torrent, sizeof(uchar), MAX_BENCODED_TORRENT_LEN, torrent_file);
    fclose(torrent_file);

    printf("%s\n", bencode_torrent);
    b_print(bencode_torrent, bencode_torrent_len);

    printf("\n%d\n", download_file(bencode_torrent, bencode_torrent_len));

    return 0;
}
 
 
// TODO general
// - add checking if HTTP response is "OK" and only then proceed to extract the bencode body
// - add documentation for all functions
// - add asserts
// - update doxygen function briefs
// - add logging to a file (and then open the log file in real time)
// - change some defines (#define) to enums to ensure that they are used in the right circumstances (enums are checked for type and defines are not)
// - when splitting functions into smaller -> reduce the names of variables to not include the function name (e.g. int request_index; ->split-> send_request() {int index; })
// - when changing signed to unsigned check all references and if they are using subtraction
// - when change ints and chars to unsigned when signed is not needed
// - almost always expect a CHOKE message when receiving
// - change returns of function from int to enum types (multiple functions can share an enum type where the enum values are named in an abstract way) (still use -1) and change integer values to enum names
// - add a `filled` field to ResourceBlock that indicates a block is filled with data

// MEM - means that a variable / statement allocated heap memory that needs to be freed later
// MEM err - memory error

// Conventions:
// *_str means a string ending with a null terminator 
// *_data means some kind of data string that doesn't have to end with a null terminator