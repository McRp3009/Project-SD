/**
 * Martim Pereira fc58223
 * João Pereira fc58189
 * Daniel Nunes fc58257
 *
 * Grupo 04
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <netdb.h>
#include "data.h"
#include "entry.h"
#include "client_stub-private.h"
#include "utils-private.h"
#include "network_client.h"
#include "sdmessage.pb-c.h"
#include "client_stub.h"
#include <sys/time.h>
#include "zookeeper/zookeeper.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int write_all(int sock, char *buf, int len)
{
    int bufsize = len;
    while (len > 0)
    {
        int res = write(sock, buf, len);
        if (res < 0)
        {
            if (errno == EINTR)
                continue;
            perror("write failed:");
            return res;
        }
        if (res == 0)
        {
            return res;
        }
        buf += res;
        len -= res;
    }
    return bufsize;
}

int read_all(int sock, char *buf, int len)
{
    int bufsize = len;
    while (len > 0)
    {
        int res = read(sock, buf, len);
        if (res < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("read failed:");
            return res;
        }
        if (res == 0)
        {
            return res;
        }
        buf += res;
        len -= res;
    }
    return bufsize;
}
/**
 *
 */
struct timeval get_time_diff(struct timeval time_end, struct timeval time_start)
{
    struct timeval time_diff;

    time_diff.tv_sec = time_end.tv_sec - time_start.tv_sec;
    time_diff.tv_usec = time_end.tv_usec - time_start.tv_usec;

    if (time_diff.tv_usec < 0)
    {
        time_diff.tv_sec -= 1;
        time_diff.tv_usec += 1000000;
    }

    return time_diff;
}