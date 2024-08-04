#include "network.h"

// waits until connected with custom timeout (if timeout is too high then the connect() may time out quicker)
// normally connect() timeouts with a OS specified time (usually ~20s)
// returns 0 on successfull connection
int connect_timeout(int fd, const struct sockaddr *addr, socklen_t len, struct timeval* timeout)
{
    // set option to not block the socket on operations:
    int flags = fcntl(fd, F_GETFL, 0); // get flags
    fcntl(fd, F_SETFL, flags | O_NONBLOCK); // set flags

    if(connect(fd, addr, len) == 0)
    {
        return -1;
    }

    if(errno != EINPROGRESS) // EINPROGRESS expected
    {
        return -1;
    }

    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);

    fd_set reads = set, writes = set;
    select(fd + 1, &reads, &writes, 0, timeout);

    // return value of send() in this line would indicate if we connected successfully

    int ret = 1; // timeout

    if(FD_ISSET(fd, &writes))
    {
        if(FD_ISSET(fd, &reads))
        {
            // check for errors
            int option_value, option_len;
            option_len = sizeof(option_value);

            int getsockopt_ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &option_value, &option_len);

            if(getsockopt_ret == -1)
            {
                ret = -1;
            }
            else
            {
                errno = option_value; // retrieve errno after getsockopt cleared it
                if(option_value == 0)
                {
                    ret = 0;
                }
                else
                {
                    ret = -1;
                }
            }
        }
        else
        {
            ret = 0; // connected
        }
    }

    fcntl(fd, F_SETFL, flags); // set flags back -> to start blocking the socket again

    return ret;
}

// send without the danger of a deadlock (deadlock can occur when two machines wait to receive ACK after both sending parts of send() data to each other)
void send_no_deadlock(int fd, const void *buf, size_t n, int flags, int (*on_recv)(void *, size_t), char* recv_buffer, size_t recv_buffer_len)
{
    fd_set zero_set;
    FD_ZERO(&zero_set);

    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);

    // send parts of data and check for recv() in between send()
    int total_bytes_sent = 0;
    while(1)
    {
        int bytes_sent = send(fd, buf + bytes_sent, n - bytes_sent, flags);
        if(bytes_sent == -1)
        {
            failure("send");
        }
        total_bytes_sent += bytes_sent;

        if(total_bytes_sent == n)
        {
            return;
        }

        fd_set reads = zero_set, writes = zero_set;

        // handle recv first and then check for ability to write (send)
        while(!FD_ISSET(fd, &writes))
        {
            reads = set;
            writes = set;
            select(fd + 1, &reads, &writes, 0, 0);
            if(FD_ISSET(fd, &reads))
            {
                // recv(fd, recv_buffer, recv_buffer_len, 0);
                on_recv(recv_buffer, recv_buffer_len);
            }
        }
    }
}

int http_response_extract_body(uchar* http_data, int http_data_len, uchar* body, int* body_len_out)
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
        *body_len_out = body_len;
        return 0;
    }

    return -1;
}

void http_explicit_hex(uchar* data, int data_len, uchar* out)
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

int start_TCP_connection(char* remote_domain_name, char* remote_port, char* local_addr_out, char* local_port_out, int* socket_out, struct timeval* timeout)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* remote_addrinfo;
    if(getaddrinfo(remote_domain_name, remote_port, &hints, &remote_addrinfo) != 0)
    {
        return -1;
    }

    uchar remote_host_buf[MAX_STR_LEN];
    uchar remote_serv_buf[MAX_STR_LEN];
    if(getnameinfo(remote_addrinfo->ai_addr, remote_addrinfo->ai_addrlen, remote_host_buf, sizeof(remote_host_buf), remote_serv_buf, sizeof(remote_serv_buf), NI_NUMERICHOST) != 0)
    {
        return -1;
    }

    int peer_socket = socket(remote_addrinfo->ai_family, remote_addrinfo->ai_socktype, remote_addrinfo->ai_protocol);
    if(socket == -1)
    {
        return -1;
    }

    int ret = connect_timeout(peer_socket, remote_addrinfo->ai_addr, remote_addrinfo->ai_addrlen, timeout);
    if(ret != 0)
    {
        return ret;
    }

    struct sockaddr local_addr;
    socklen_t local_addr_len = sizeof(local_addr);
    if(getsockname(peer_socket, &local_addr, &local_addr_len) == -1)
    {
        return -1;
    }

    uchar local_host_buf[MAX_STR_LEN];
    uchar local_serv_buf[MAX_STR_LEN];
    if(getnameinfo(&local_addr, local_addr_len, local_host_buf, sizeof(local_host_buf), local_serv_buf, sizeof(local_serv_buf), NI_NUMERICHOST | NI_NUMERICSERV) != 0)
    {
        return -1;
    }

    strcpy(local_addr_out, local_host_buf);
    strcpy(local_port_out, local_serv_buf);

    LOG(printf("Connected: %s (%s) -> %s (%s)\n", local_host_buf, local_serv_buf, remote_host_buf, remote_serv_buf);)

    freeaddrinfo(remote_addrinfo);

    *socket_out = peer_socket;

    return 0;
}


// TODO document functions (do it using some standard/extension)