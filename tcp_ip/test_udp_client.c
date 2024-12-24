#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

 #define SERVER_PORT 8888
 #define BUFF_LEN 512
 #define SERVER_IP "172.0.5.182"


void udp_msg_sender(int fd,  char *ip, int port)
{
    struct sockaddr_in dst;

    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = inet_addr(ip);
    ///dst.sin_addr.s_addr = htonl(INADDR_ANY);  //注意网络序转换
    dst.sin_port = htons(port);  //注意网络序转换


    char ip_str[BUFF_LEN] = {0};

    socklen_t len = sizeof(dst);
    struct sockaddr_in src;
    for (int i = 0; i < 10; ++i) {
        char buf[BUFF_LEN] = "TEST UDP MSG!\n";
        printf("client:%s\n",buf);  //打印自己发送的信息
        sendto(fd, buf, strlen(buf), 0, &dst, len);
        memset(buf, 0, sizeof(buf));
        recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&src, &len);  //接收来自server的信息
        // 将 sockaddr_in 转换为字符串
        inet_ntop(AF_INET, &(src.sin_addr), ip_str, BUFF_LEN);
        printf("rcv from server(%s:%d), [%s]\n", ip_str, ntohs(src.sin_port), buf);
        sleep(1);  //一秒发送一次消息
    }
}

/*
    client:
            socket-->sendto-->revcfrom-->close
*/

int main(int argc, char* argv[])
{
    int client_fd;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    char *ipA = "43.128.51.86";
    char *ipB = "64.176.173.228";

    udp_msg_sender(client_fd,  ipA, SERVER_PORT);
    udp_msg_sender(client_fd,  ipB, SERVER_PORT);

    close(client_fd);

    return 0;
}
