#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#define SERVER_PORT 8888
#define BUFF_LEN 1024

// NOTE: linux 的系统调用都具有原子性，故而无需对 server_fd 的操作 加锁

static int server_fd = -1;

void *handle_rcv_udp_msg(void *)
{
    printf("into handle_rcv_udp_msg\n");
    char buf[BUFF_LEN];  //接收缓冲区，1024字节
    int count;
    struct sockaddr_in clent_addr;  //clent_addr用于记录发送方的地址信息
    while(1)
    {
        char ip_str[BUFF_LEN] = {0};

        memset(buf, 0, BUFF_LEN);
        socklen_t len = sizeof(clent_addr);
        count = recvfrom(server_fd, buf, BUFF_LEN, 0, (struct sockaddr*)&clent_addr, &len);  //recvfrom是拥塞函数，没有数据就一直拥塞
        if(count == -1)
        {
            printf("recieve data fail!\n");
            return;
        }
        inet_ntop(AF_INET, &(clent_addr.sin_addr), ip_str, BUFF_LEN);

        printf("RCV FROM POINT(%s:%d):[%s]\n", ip_str, ntohs(clent_addr.sin_port), buf);  //打印client发过来的信息
        memset(buf, 0, BUFF_LEN);
        sleep(1);
        sprintf(buf, "hi I have recieved %d bytes data!\n", count);  //回复client
        printf("SEND TO POINT:%s\n",buf);  //打印自己发送的信息给
        sendto(server_fd, buf, BUFF_LEN, 0, (struct sockaddr*)&clent_addr, len);  //发送信息给client，注意使用了clent_addr结构体指针
    }
}

void handle_udp_send_msg(char *ip, int port)
{
    struct sockaddr_in dst;

    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = inet_addr(ip);
    ///dst.sin_addr.s_addr = htonl(INADDR_ANY);  //注意网络序转换
    dst.sin_port = htons(port);  //注意网络序转换

    socklen_t len = sizeof(dst);

    char buf[BUFF_LEN] = "UDP SERVER SEND TO CLIENT BY SELF!\n";
    printf("client:%s\n",buf);  //打印自己发送的信息
    sendto(server_fd, buf, strlen(buf), 0, &dst, len);
}

int main(int argc, char* argv[])
{
    int ret;
    struct sockaddr_in ser_addr; 

    server_fd = socket(AF_INET, SOCK_DGRAM, 0); //AF_INET:IPV4;SOCK_DGRAM:UDP
    if(server_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    ser_addr.sin_port = htons(SERVER_PORT);  //端口号，需要网络序转换

    ret = bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
    if(ret < 0)
    {
        printf("socket bind fail!\n");
        return -1;
    }

    pthread_t io_p;
    pthread_create(&io_p, NULL, handle_rcv_udp_msg, NULL);

    sleep(10);
    char *ip =  "43.128.51.86";
    handle_udp_send_msg(ip, SERVER_PORT);

    sleep(1000);

    close(server_fd);
    return 0;
}
