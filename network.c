#include "network.h"

void failure(char* function_name)
{
    fprintf(stderr, "%s() failed (%d) -> [%s]\n", function_name, errno, strerror(errno));

    exit(1);
}

int explicit_str(char* src, int len, char* dst)
{
    char* escape_str[] = {"\\0", "\\a", "\\b", "\\e", "\\f", "\\n", "\\r", "\\t", "\\v", "\\\\", "\\'", "\\\"", "\\?"};
    char escape_char[] = "\0\a\b\e\f\n\r\t\v\\\'\"\?";
    int escape_len = sizeof(escape_char);

    int k = 0;
    for(int i = 0; i < len; i++)
    {
        int is_escape_char = 0;
        for(int j = 0; j < escape_len; j++)
        {
            if(src[i] == escape_char[j])
            {
                is_escape_char = 1;
                strcpy(dst + k, escape_str[j]);
                k++;
                break;
            }
        }

        if(!is_escape_char)
        {
            dst[k] = src[i];
        }

        k++;
    }

    return k;
}

// waits until connected with custom timeout (if timeout is too high then the connect() may time out quicker)
// normally connect() timeouts with a OS specified time (usually ~20s)
int connect_timeout(int fd, const struct sockaddr *addr, socklen_t len, struct timeval timeout)
{
    // set option to not block the socket on operations:
    int flags = fcntl(fd, F_GETFL, 0); // get flags
    fcntl(fd, F_SETFL, flags | O_NONBLOCK); // set flags

    int err = connect(fd, addr, len);
    if(err != EINPROGRESS) // EINPROGRESS expected
    {
        failure("connect");
    }

    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);

    fd_set reads = set, writes = set;
    select(fd + 1, &reads, &writes, 0, &timeout);

    // return value of send() in this line would indicate if we connected successfully

    int ret = 1;

    if(FD_ISSET(fd, &writes))
    {
        if(FD_ISSET(fd, &reads))
        {
            // check for errors
            int option_value, option_len;
            option_len = sizeof(option_value);
            if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &option_value, &option_len) == -1)
            {
                failure("getsockopt");
            }

            if(option_value == 0)
            {
                ret = 0;
            }
            else
            {
                ret = 1;
            }
        }
        else
        {
            ret = 0; // connected
        }

        ret = 0;
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