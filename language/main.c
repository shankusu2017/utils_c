#include <stdio.h>
#include <stdint.h>

// /* 协议头 */
// typedef struct ipc_proto_header_s {
//     int ttl;           /* 原始的数据长度(不含协议头) */
//     int msg_type;
//     int seq_id;        /* 发送者这边的发送编号 */
//     int ack_id;        /* 对某个消息的回复编号(0:无效) */
// } ipc_proto_header_t;

// /* 消息结构体
//  * 不能插入其它结构体
//  */
// typedef struct ipc_packet_s {
//     ipc_proto_header_t head;	/* 协议头 */
//     char buf[];                 /* 原始的负载 */
// } ipc_packet_t;


// typedef struct packet_io_s {
//     int fd;
// } packet_io_t;

// /* 用于发送数据的结构体 */
// typedef struct packet_send_s {
//     uint64_t h;
//     packet_io_t io;
//     ipc_packet_t head;	/* 协议头 */
// } packet_send_t;

// int main(int argc, char *argv[]) 
// {
//     printf("%x\n", sizeof(packet_send_t));
//     packet_send_t *pkt =(packet_send_t*)malloc(sizeof(packet_send_t) + 100);
//     if (NULL == pkt) {
//         exit(-1);
//     }

//     printf("%x\n", pkt);
//     printf("%x\n", pkt->head.buf);
//     printf("%x\n", (char *)pkt + sizeof(packet_send_t));

//     return 0;
// }