#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define SERVER_PORT 40151
#define BUFF_LEN 1024

// NOTE: linux 的系统调用都具有原子性，故而无需对 server_fd 的操作 加锁
static int server_fd = -1;
static int rcv_ttl = 0;

void *handle_rcv_udp_msg(void *arg)
{
    (void)arg;

    printf("into handle_rcv_udp_msg\n");
    char buf[BUFF_LEN];  //接收缓冲区，1024字节
    int count;
    struct sockaddr_in clent_addr;  //clent_addr用于记录发送方的地址信息
    socklen_t len = sizeof(clent_addr);
    while (1) {
        count = recvfrom(server_fd, buf, BUFF_LEN, 0, (struct sockaddr*)&clent_addr, &len);  //recvfrom是拥塞函数，没有数据就一直拥塞
        if (count == -1) {
            printf("recieve data fail!\n");
            continue;
        }
        sendto(server_fd, buf, count, 0, (struct sockaddr*)&clent_addr, len);  //发送信息给client，注意使用了clent_addr结构体指针
        rcv_ttl++;
    }
}

static void * handle_printf(void *arg)
{
    (void)arg;
    while (1) {
        sleep(1);
        printf("rcv.ttl: %d\n", rcv_ttl);
    }
}

int main(int argc, char* argv[])
{
    int ret;
    struct sockaddr_in ser_addr;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0); //AF_INET:IPV4;SOCK_DGRAM:UDP
    if (server_fd < 0) {
        printf("create socket fail!\n");
        return -1;
    }

    {
        int bufsize = 0;
        socklen_t optlen = sizeof(bufsize);
        if (getsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, &optlen) < 0) {
            perror("getsockopt");
            exit(EXIT_FAILURE);
        }
        printf("old.rcvbuf: %d\n", bufsize);


        bufsize = 64 * 1024 * 1024;
        // 设置缓冲区大小
        if (setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) < 0) {
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }
        if (setsockopt(server_fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) < 0) {
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }
        bufsize = 0;
        if (getsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, &optlen) < 0) {
            perror("getsockopt");
            exit(EXIT_FAILURE);
        }
        printf("new.rcvbuf: %d\n", bufsize);
        bufsize = 0;
        if (getsockopt(server_fd, SOL_SOCKET, SO_SNDBUF, &bufsize, &optlen) < 0) {
            perror("getsockopt");
            exit(EXIT_FAILURE);
        }
        printf("new.sendbuf: %d\n", bufsize);
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    ser_addr.sin_port = htons(SERVER_PORT);  //端口号，需要网络序转换

    ret = bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
    if (ret < 0) {
        printf("socket bind fail!\n");
        return -1;
    }

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, handle_rcv_udp_msg, NULL);
    pthread_create(&thread_id, NULL, handle_printf, NULL);

    sleep(10000000);
    close(server_fd);
    return 0;
}
