#ifndef IPC_COMMON_H_20240821113750_0X77F3ACE5
#define IPC_COMMON_H_20240821113750_0X77F3ACE5


#include "common.h"
#include "threadpool.h"
#include "hash.h"
#include "tcp_ip.h"
#include "time.h"
#include "ipc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ipc_connect_status_e {
    ipc_connect_status_done,
    ipc_connect_status_fail
} ipc_connect_status_t;

typedef struct ipc_client_handler_s {
    char server_ip[IPV4_LEN_STR_00];
    uint16_t server_port;

    pthread_rwlock_t rwlock;     /* lock ----------------> */
    ipc_connect_status_t status;
    int sockfd;                  /* lock <---------------- */


    pthread_mutex_t mutex;      /* mutex ----------------> */
    pthread_cond_t cond;
    hash_table_t *seq_hash;     /* 已收到消息的哈西表(限制：send_seq 生成，wait_seq 使用) */
    uint64_t     seq_id;        /* 下一条发送的消息ID */
                                /* mutex <---------------- */
    threadpool_t *thread;

    uint64_t rcv_byte_ttl;
    uint64_t rcv_pkt_ttl;
    uint64_t send_byte_ttl;
    uint64_t send_pkt_ttl;
} ipc_client_handler_t;


typedef struct ipc_cli_s ipc_cli_t;


typedef struct ipc_server_handler_s {
    int sockfd;
    ipc_cli_t *sock_cli;        	 /* 当做一个特殊的client, 不放入 cli.info.hash  */

    int epoll;

    hash_table_t *cli_info_hash;     /* 客户端hash(uuid->client.info),读线程独占，无需mutex */


    pthread_mutex_t cli_fd_mutex;               /* lock ----------------> */
    hash_table_t *cli_fd_hash;                  /* 客户端hash(uuid->fd) */
                                                /* lock ----------------> */

    uint64_t            seq_id;        /* 下一条发送的消息ID */
    pthread_mutex_t     seq_mutex;

    threadpool_t *thread;

    ipc_callback cb;            /* 消息回调函数 */
    int pipe[2];                /* recv->callback */

    /* 统计用 */
    uint64_t rcv_byte_ttl;
    uint64_t rcv_pkt_ttl;
    uint64_t send_byte_ttl;
    uint64_t send_pkt_ttl;
} ipc_server_handler_t;





//////////////////////////////// protocol ////////////////////////////////

/* 协议头 */
typedef struct ipc_proto_header_s {
    uint64_t ttl;           /* 原始的数据长度(不含协议头) */
    ipc_msg_type_t msg_type;
    uint64_t seq_id;        /* 发送者这边的发送编号 */
    uint64_t ack_id;        /* 对某个消息的回复编号(0:无效) */
} ipc_proto_header_t;

/* 协议结构体 */
typedef struct ipc_proto_s {
    ipc_proto_header_t head;	/* 协议头 */
    char buf[];                 /* 原始的负载 */
} ipc_proto_t;


#define IPC_SEQ_ID_MIN 10000ULL     /* 保留 [1, 9999] 之间的 id， 0：ZERO-VALUE */
#define IPC_SEQ_ID_DEBUG 0xffffffffffffffffULL
#define IPC_ACK_ID_NULL 0           /* 无效的 ack_id */
//////////////////////////////// protocol ////////////////////////////////

/* 一次读取的数据长度 */
#define IPC_MSG_READ_BLOCK_LEN_MAX (1024*16)



typedef struct ipc_cli_s {
    int fd;                         /* 客户端fd */
    struct sockaddr_in addr;        /* 客户端地址 */
    uint64_t uuid;                  /* 客户端的唯一标识符 */

    /* 读数据后，解析协议相关 */
    size_t has_read;                /* 本次数据包已读取的字节数(包含协议头在内) */
    int head_done;                  /* 协议头，buf 是否读取完毕 */
    int buf_done;
    ipc_proto_header_t ipc_head;    /* 协议头*/
    void *buf;                      /* 负载 */
} ipc_cli_t;

typedef struct ipc_pipe_data_s {
    uint64_t cli_uuid;                  /* 客户端的唯一标识符，在回调线程中回写数据时，需要通过 uuid 找到 cli 的实时信息(主要是fd) */
    ipc_proto_header_t ipc_head;        /* 消息头 */
    char *buf;                          /* 消息负载 */
    void *return_addr;
    size_t return_len;
} ipc_pipe_data_t;


#ifndef IPC_SERVER_CLIENT_MAP_SIZE
#define IPC_SERVER_CLIENT_MAP_SIZE (1024)
#endif

#ifndef IPC_CLIENT_SEQ_HASH_SIZE
#define IPC_CLIENT_SEQ_HASH_SIZE (16*1024)
#endif

#ifndef IPC_CLIENT_ACK_WAIT
#define IPC_CLIENT_ACK_WAIT (3*1000)
#endif



#ifdef __cplusplus
}
#endif

#endif  /* IPC_COMMON_H_20240821113750_0X77F3ACE5 */
