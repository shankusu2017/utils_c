#include "common.h"
#include "tcp_ip.h"

int uc_setnonblocking(int sockfd)
{
    int ret = fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK);
    return ret;
}

int uc_setnodelay(int sockfd)
{
    int on = 1;
    int ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
    return ret;
}

int uc_setreuse(int sockfd)
{
    socklen_t on = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    return ret;
}

int uc_setsndbuf(int sockfd, int len)
{
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &len, sizeof(len));
    if (ret == -1) {
        perror("setsockopt");
    }

    return ret;
}

int uc_setrcvbuf(int sockfd, int len)
{
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
    if (ret == -1) {
        perror("setsockopt");
    }

    return ret;
}
