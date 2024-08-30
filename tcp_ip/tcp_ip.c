#include "common.h"
#include "tcp_ip.h"

int setnonblocking(int sockfd)
{
    int ret = fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK);
    return ret;
}

int setnodelay(int sockfd)
{
    int on = 1;
    int ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
    return ret;
}

int setreuse(int sockfd)
{
    socklen_t on = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    return ret;
}

int utils_setsndbuf(int sockfd, int len)
{
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &len, sizeof(len));
    if (ret == -1) {
        perror("setsockopt");
    }

    return ret;
}

int utils_setrcvbuf(int sockfd, int len)
{
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
    if (ret == -1) {
        perror("setsockopt");
    }

    return ret;
}
