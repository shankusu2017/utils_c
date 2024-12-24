#ifndef IPC_COMMON_H_20240821113750_0X77F3ACE5
#define IPC_COMMON_H_20240821113750_0X77F3ACE5


#include "util.h"
#include "threadpool.h"
#include "hash.h"
#include "tcp_ip.h"
#include "time.h"
#include "ipc.h"
#include "random.h"
#include "log.h"


#define IPC_DEBUG_IO 1
//#define IPC_CLIENT_SEND_THREADS 1


#ifdef __cplusplus
extern "C" {
#endif

typedef enum uc_ipc_connect_status_e {
    uc_ipc_connect_status_done,
    uc_ipc_connect_status_fail
} uc_ipc_connect_status_t;

typedef struct uc_ipc_client_handler_s {
    char server_ip[UC_IPV4_LEN_STR_00];
    uint16_t server_port;

    uc_ipc_connect_status_t io_status;
    int io_sockfd;
    pthread_mutex_t io_mtx;         /* IO写的互斥(单次发送不能被插队) */

    uint64_t     seq_id;            /* 下一条发送的消息ID */
    pthread_mutex_t seq_mtx;        /* IO写的互斥(单次发送不能被插队) */

    pthread_cond_t ack_cond;
    uc_hash_table_t *ack_hash;         /* 已收到消息的哈西表(限制：send_seq 生成，wait_seq 使用) */
    pthread_mutex_t ack_mtx;

    uc_threadpool_t *thread;
} uc_ipc_client_handler_t;


typedef struct uc_ipc_cli_s uc_ipc_cli_t;


typedef struct uc_ipc_server_handler_s {
    int sockfd;
    uc_ipc_cli_t *sock_cli;        	 /* 当做一个特殊的client, 不放入 cli.info.hash  */

    int epoll;

    uc_hash_table_t *cli_info_hash;     /* 客户端hash(uuid->client.info),读线程独占，无需mutex */


    pthread_mutex_t cli_fd_mutex;               /* lock ----------------> */
    uc_hash_table_t *cli_fd_hash;                  /* 客户端hash(uuid->fd) */
                                                /* lock ----------------> */

    uint64_t            seq_id;        /* 下一条发送的消息ID */
    pthread_mutex_t     seq_mutex;

    uc_threadpool_t *thread;

    uc_ipc_callback cb;            /* 消息回调函数 */
    int pipe[2];                /* recv->callback */

    /* 统计用 */
    uint64_t rcv_byte_ttl;
    uint64_t rcv_pkt_ttl;
    uint64_t send_byte_ttl;
    uint64_t send_pkt_ttl;
} uc_ipc_server_handler_t;





//////////////////////////////// protocol ////////////////////////////////

/* 协议头 */
typedef struct uc_ipc_proto_header_s {
    uint64_t ttl;           /* 原始的数据长度(不含协议头) */
    uc_ipc_msg_type_t msg_type;
    uint64_t seq_id;        /* 发送者这边的发送编号 */
    uint64_t ack_id;        /* 对某个消息的回复编号(0:无效) */
}  __attribute__ ((packed)) uc_ipc_proto_header_t;

/*
 * 协议结构体
 * 不能插入其它结构体
 */
typedef struct uc_ipc_proto_packet_s {
    uc_ipc_proto_header_t head;	/* 协议头 */
    char buf[];                 /* 原始的负载 */
}  __attribute__ ((packed)) uc_ipc_proto_packet_t;


#define UC_IPC_SEQ_ID_MIN 10000ULL     /* 保留 [1, 9999] 之间的 id， 0：ZERO-VALUE */
#define UC_IPC_SEQ_ID_DEBUG 0xffffffffffffffffULL
#define UC_IPC_ACK_ID_NULL 0           /* 无效的 ack_id */
//////////////////////////////// protocol ////////////////////////////////



/* 一次读取的数据长度 */
#define UC_IPC_MSG_READ_BLOCK_LEN_MAX (1024*8)
#define UC_IPC_SOCK_BUF_LEN (8*1024*1024)


typedef struct uc_ipc_cli_s {
    int fd;                         /* 客户端fd */
    struct sockaddr_in addr;        /* 客户端地址 */
    uint64_t uuid;                  /* 客户端的唯一标识符 */

    /* 读数据后，解析协议相关 */
    size_t has_read;                /* 本次数据包已读取的字节数(包含协议头在内) */
    int head_done;                  /* 协议头，buf 是否读取完毕 */
    int buf_done;
    uc_ipc_proto_header_t ipc_head;    /* 协议头*/
    void *buf;                      /* 负载 */
} uc_ipc_cli_t;

typedef struct uc_ipc_pipe_data_s {
    uint64_t cli_uuid;                  /* 客户端的唯一标识符，在回调线程中回写数据时，需要通过 uuid 找到 cli 的实时信息(主要是fd) */
    uc_ipc_proto_header_t ipc_head;        /* 消息头 */
    char *buf;                          /* 消息负载 */
    void *return_addr;
    size_t return_len;
} uc_ipc_pipe_data_t;

typedef struct uc_packet_io_s {
    int fd;
} __attribute__ ((packed)) uc_packet_io_t;

/* 用于发送数据的结构体 */
typedef struct uc_packet_send_s {
    uc_packet_io_t io;             /* io 域 */
    uc_ipc_proto_packet_t *proto_packet; /* 协议头+原始的负载 */
} __attribute__ ((packed)) uc_packet_send_t;


#ifndef UC_IPC_SERVER_CLIENT_MAP_SIZE
#define UC_IPC_SERVER_CLIENT_MAP_SIZE (1024)
#endif

#ifndef UC_IPC_CLIENT_SEQ_HASH_SIZE
#define UC_IPC_CLIENT_SEQ_HASH_SIZE (16*1024)
#endif

#ifndef UC_IPC_CLIENT_ACK_WAIT
#define UC_IPC_CLIENT_ACK_WAIT (3*1000)
#endif

#ifndef UC_IPC_CLIENT_THREAD_POOL_MSG_WAIT_MAX
#define UC_IPC_CLIENT_THREAD_POOL_MSG_WAIT_MAX 4096
#endif

#ifdef __cplusplus
}
#endif

#endif  /* IPC_COMMON_H_20240821113750_0X77F3ACE5 */
