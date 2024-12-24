#include "ipc.h"
#include "ipc_common.h"
#include "ipc_io.h"

static void *uc_ipc_client_loop_rcv(void *arg);

static uint64_t uc_ipc_client_next_seq_id(uc_ipc_client_handler_t *hdl)
{
    uint64_t next = 0;
    pthread_mutex_lock(&hdl->seq_mtx);
    next = hdl->seq_id;
    hdl->seq_id++;
    if (hdl->seq_id < UC_IPC_SEQ_ID_MIN) {
        hdl->seq_id = UC_IPC_SEQ_ID_MIN;
    }
    pthread_mutex_unlock(&hdl->seq_mtx);

    return next;
}

/* 插入一个 seq 数据（等待对方 ack 的到来）*/
static int uc_ipc_client_insert_seq(uc_ipc_client_handler_t *hdl, uint64_t seq_id)
{
    int ret = 0;
    uc_hash_key_t key = {.u64 = seq_id};

    pthread_mutex_lock(&hdl->ack_mtx);
    ret = uc_hash_insert(hdl->ack_hash, key, NULL);
    pthread_mutex_unlock(&hdl->ack_mtx);

    if (ret) {
        printf("0x4cd98a52 insert seq fail, ret: %d", ret);
    }

    return ret;
}

/* 已经收到并处理了对方的 ack(或者ack对应的msg发送失败等原因), 这里删除 ack 相关数据 */
static void uc_ipc_client_delete_seq(uc_ipc_client_handler_t *hdl, uint64_t seq_id)
{
    int ret = 0;
    uc_hash_key_t key = {.u64 = seq_id};

    pthread_mutex_lock(&hdl->ack_mtx);
    ret = uc_hash_delete(hdl->ack_hash, key);
    pthread_mutex_unlock(&hdl->ack_mtx);

    if (ret) {
        printf("error 0x4858115b delete fail, ret: %d", ret);
    }
}

static void *uc_ipc_client_write(void *data)
{
    static size_t send_ttl = 0;

    uc_packet_send_t *msg = (uc_packet_send_t *)data;
    size_t len = sizeof(uc_ipc_proto_packet_t) + msg->proto_packet->head.ttl;
    int ret = uc_io_write(msg->io.fd, msg->proto_packet, len);
    if (ret) {
        log("0x787f6f9e send fail, ret: %d\n", ret);
        goto write_done;
    } else {
        //printf("0x57246fae send done, len: %lu\n", len);
    }

    if (++send_ttl % 10000 == 0) {
        log("0x5d29f23e send.ttl: %ld");
    }

write_done:
    free(msg->proto_packet);
    free(msg);
    msg = NULL;
    return NULL;
}

static int uc_ipc_client_send(uc_ipc_client_handler_t *hdl, uint64_t seq_id, uc_ipc_msg_type_t msg_type, void *buf, size_t len)
{
    uc_ipc_proto_packet_t *proto_pkt = (uc_ipc_proto_packet_t*)malloc(sizeof(uc_ipc_proto_packet_t) + len);
    if (NULL == proto_pkt) {
        printf("0x1f9385ca malloc fail at ipc client send ...\n");
        return -0x1f9385ca;
    }
    uc_packet_send_t *msg = (uc_packet_send_t*)malloc(sizeof(uc_packet_send_t));
    if (NULL == msg) {
        free(proto_pkt);
        proto_pkt = NULL;
        printf("0x3cbe5a0f malloc fail at ipc client send ...\n");
        return -0x3cbe5a0f;
    }

    // PROTOCOL 相关代码， 后同
    proto_pkt->head.ttl = len;
    proto_pkt->head.msg_type = msg_type;
    proto_pkt->head.seq_id = seq_id;
    proto_pkt->head.ack_id = UC_IPC_ACK_ID_NULL;
    if (len) {
        memcpy(proto_pkt->buf, buf, len);
    }

    msg->io.fd = hdl->io_sockfd;
    msg->proto_packet = proto_pkt;

#ifdef IPC_CLIENT_SEND_THREADS
    uc_threadpool_add_void_task(hdl->thread, uc_ipc_client_write, msg);
#else
    pthread_mutex_lock(&hdl->io_mtx);
    uc_ipc_client_write(msg);
    pthread_mutex_unlock(&hdl->io_mtx);
#endif

    return 0;
}

/*
 * 建立于 server 的链接
 * server_ip: 127.0.0.1
 * server_port: 80
 */
uc_ipc_client_handler_t *uc_ipc_init_client(char *server_ip, uint16_t server_port)
{
    uc_ipc_client_handler_t *hdl = calloc(1, sizeof(uc_ipc_client_handler_t));
    if (NULL == hdl) {
        return NULL;
    }

    // fcntl(hdl->pipe[0], F_SETFL, O_NOATIME); /* 提高性能 */
    // fcntl(hdl->pipe[1], F_SETFL, O_NOATIME);

    if (pthread_mutex_init(&hdl->io_mtx, NULL) != 0) {
        free(hdl);
        hdl = NULL;
        return hdl;
    }

    if (pthread_mutex_init(&hdl->seq_mtx, NULL) != 0) {
        free(hdl);
        hdl = NULL;
        return hdl;
    }

    if (pthread_mutex_init(&hdl->ack_mtx, NULL) != 0) {
        free(hdl);
        hdl = NULL;
        return hdl;
    }
    if (pthread_cond_init(&hdl->ack_cond, NULL) != 0) {
        pthread_mutex_destroy(&hdl->ack_mtx);
        free(hdl);
        hdl = NULL;
        return hdl;
    }

    strncpy(hdl->server_ip, server_ip, ARRAY_SIZE(hdl->server_ip));
    hdl->server_port = server_port;
    hdl->io_status = uc_ipc_connect_status_fail;
    hdl->io_sockfd = -1;
    hdl->ack_hash = NULL;
    hdl->thread = NULL;

    hdl->seq_id = UC_IPC_SEQ_ID_MIN;
    int ret = uc_urandom(&hdl->seq_id, sizeof(hdl->seq_id));
    if (0 != ret) {
        goto err_uninit;
    }
    if (hdl->seq_id < UC_IPC_SEQ_ID_MIN) {
        hdl->seq_id = UC_IPC_SEQ_ID_MIN;
    }

    hdl->io_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (hdl->io_sockfd == -1) {
        printf("-0x5e173d0a socket creation failed...\n");
        goto err_uninit;
    }
    ret = uc_setnodelay(hdl->io_sockfd);    /* ipc 基本用于本机通信，效率为主 */
    if (ret) {
        printf("0x1ec22172 set nodelay fail, ret: %d\n", ret);
    }
    uc_setrcvbuf(hdl->io_sockfd , UC_IPC_SOCK_BUF_LEN);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(server_ip);
    servaddr.sin_port = htons(server_port);
    if (connect(hdl->io_sockfd , (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        printf("0x603ea840 connection failed...\n");
        hdl->io_status = uc_ipc_connect_status_fail;
        goto err_uninit;    /* NOTE: 初始化阶段，链接失败直接返回，后续通信过程中，失败可以 retry */
    } else {
        hdl->io_status = uc_ipc_connect_status_done;
    }

    hdl->ack_hash = uc_hash_create(UC_IPC_CLIENT_SEQ_HASH_SIZE, uc_hash_key_uint64, 0);
    if (NULL == hdl->ack_hash) {
        goto err_uninit;
    }

    hdl->thread = uc_threadpool_create(2, 2);   /* 1 rcv, 1 send */
    if (NULL == hdl->thread) {
        goto err_uninit;
    }
    uc_threadpool_set_task_max(hdl->thread, UC_IPC_CLIENT_THREAD_POOL_MSG_WAIT_MAX);
    uc_threadpool_add_void_task(hdl->thread, uc_ipc_client_loop_rcv, hdl);
    // 等待 接收线程跑起来
    uc_threadpool_wait_task_done(hdl->thread);

    return hdl;

err_uninit:
    if (NULL != hdl->thread) {
        uc_threadpool_close(hdl->thread);
        hdl->thread = NULL;
    }
    if (NULL != hdl->ack_hash) {
        uc_hash_free(hdl->ack_hash);
        hdl->ack_hash = NULL;
    }
    if (-1 != hdl->io_sockfd ) {
        close(hdl->io_sockfd );
        hdl->io_sockfd  = -1;
        hdl->io_status = uc_ipc_connect_status_fail;
    }

    pthread_cond_destroy(&hdl->ack_cond);
    pthread_mutex_destroy(&hdl->ack_mtx);
    pthread_mutex_destroy(&hdl->seq_mtx);
    pthread_mutex_destroy(&hdl->io_mtx);

    free(hdl);
    hdl = NULL;
    return hdl;
}

/*
 * 关闭 socket, 开始不断的重连
*/
static void uc_ipc_client_reconnect(uc_ipc_client_handler_t *hdl, int reason) {
    printf("0x3093c4ea reconnection at: %x......\n", reason);

    pthread_mutex_lock(&hdl->io_mtx);
    close(hdl->io_sockfd);
    hdl->io_sockfd  = -1;
    hdl->io_status = uc_ipc_connect_status_fail;
    pthread_mutex_unlock(&hdl->io_mtx);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    while (fd < 0) {
        sleep(1);
        fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    pthread_mutex_lock(&hdl->io_mtx);
    hdl->io_sockfd  = fd;
    pthread_mutex_unlock(&hdl->io_mtx);
    int ret = uc_setnodelay(hdl->io_sockfd );    /* ipc 基本用于本机通信，效率为主 */
    if (ret) {
        printf("0x0d4715bc set nodelay fail, ret: %d\n", ret);
    }
    uc_setrcvbuf(hdl->io_sockfd , UC_IPC_SOCK_BUF_LEN);


    while (1) {
        pthread_mutex_lock(&hdl->seq_mtx);
        int ret = uc_urandom(&hdl->seq_id, sizeof(hdl->seq_id));
        if (ret) {
            pthread_mutex_unlock(&hdl->seq_mtx);
            sleep(1);
            printf("0x187d8b58 get random fail, ret: %d\n", ret);
        } else {
            if (hdl->seq_id < UC_IPC_SEQ_ID_MIN) {
                hdl->seq_id = UC_IPC_SEQ_ID_MIN;
            }
            pthread_mutex_unlock(&hdl->seq_mtx);
            break;
        }
    }

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(hdl->server_ip);
    servaddr.sin_port = htons(hdl->server_port);

    while (1) {
        if (connect(hdl->io_sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) {
            printf("0x603ea840 reconnection fail......\n");
            uc_time_msleep(64);
        } else {
            pthread_mutex_lock(&hdl->io_mtx);
            hdl->io_status = uc_ipc_connect_status_done;
            pthread_mutex_unlock(&hdl->io_mtx);
            break;
        }
    }
    printf("0x1159dc1d reconnection done...\n");
}

// 客户端
static void *uc_ipc_client_loop_rcv(void *arg)
{
    uc_ipc_client_handler_t *hdl = (uc_ipc_client_handler_t *)arg;
    int closed = 0;
    uc_ipc_proto_header_t head;

    while (1) {
        int ret = uc_io_read(hdl->io_sockfd , &head, sizeof(head), &closed);
        if (ret || closed) {    /* 没读完，报错，或者是被关闭等 */
            printf("0x7f839461 io.read fail, ret: %d\n", ret);
            uc_ipc_client_reconnect(hdl, 0x7f839461);  /* reconnect........ */
            continue;
        }
        if (head.seq_id < UC_IPC_SEQ_ID_MIN) {
            uc_ipc_client_reconnect(hdl, 0x3ba3bb2c);
            continue;
        }

        uc_ipc_proto_packet_t *msg = (uc_ipc_proto_packet_t*)malloc(sizeof(uc_ipc_proto_header_t) + head.ttl);
        if (NULL == msg) {
            uc_ipc_client_reconnect(hdl, 0x650beacb);
            continue;
        }
        memcpy(&msg->head, &head, sizeof(head));

        ret = uc_io_read(hdl->io_sockfd , msg->buf, head.ttl, &closed);
        if (ret || closed) {
            printf("0x17073c93 read fail, ret: %d, closed: %d\n", ret, closed);
            uc_ipc_client_reconnect(hdl, 0x17073c93);  /* reconnect........ */
            continue;
        }

        if (msg->head.msg_type == uc_ipc_msg_type_async_ack ||
            msg->head.msg_type == uc_ipc_msg_type_sync_ack) {
            if (msg->head.ack_id < UC_IPC_SEQ_ID_MIN) {
                uc_ipc_client_reconnect(hdl, 0x13a5edcb);  /* reconnect........ */
                continue;
            }

            uc_hash_key_t key = {.u64 = msg->head.ack_id};
            pthread_mutex_lock(&hdl->ack_mtx);
            uc_hash_node_t *node = uc_hash_find(hdl->ack_hash, key);
            if (NULL == node) {
                printf("0x025a4313 warn ack too later");
                pthread_cond_signal(&hdl->ack_cond);
                pthread_mutex_unlock(&hdl->ack_mtx);
                free(msg);
                msg = NULL;
            } else {
                // node 的删除，由 wait_ack 处理
                uc_hash_update(hdl->ack_hash, key, msg);
                pthread_cond_signal(&hdl->ack_cond);
                pthread_mutex_unlock(&hdl->ack_mtx);
            }
        } else {    /* 收到了不应该收到的信息类型 */
            printf("0x73676262 client rcv invalid msg(%d)\n", msg->head.msg_type);
            free(msg);
            msg = NULL;
        }
    }
}

/*
 * 外层调用通过 free_hdl 句柄释放 返回的内存
*/
static int uc_ipc_client_wait_ack(uc_ipc_client_handler_t *hdl, uint64_t seq_id, int32_t wait_millisecond, void **free_hdl, void **return_addr, size_t *return_len)
{
    if (free_hdl) {
        *free_hdl = NULL;
    }
    if (return_addr) {
        *return_addr = NULL;
    }
    if (return_len) {
        *return_len = 0;
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    struct timespec outtime;
    outtime.tv_sec = now.tv_sec + wait_millisecond/1000;
    outtime.tv_nsec = now.tv_usec * 1000 + (wait_millisecond%1000)*1000*1000;

    uc_hash_key_t key = {.u64 = seq_id};
    int ret = 0;
    pthread_mutex_lock(&hdl->ack_mtx);
    uc_hash_table_t *hash = hdl->ack_hash;
    uc_hash_node_t *node = uc_hash_find(hash, key);
    if (NULL == node) {
        pthread_mutex_unlock(&hdl->ack_mtx);
        printf("0x790396ad run error, can't find sync-ack data in hash\n");
        return -0x790396ad;
    }

    ret = 0;
    while (NULL == uc_hash_node_val(node)) {
        if (wait_millisecond == 0) {
            ret = pthread_cond_wait(&hdl->ack_cond, &hdl->ack_mtx);
        } else {
            // TODO 需要每次都更新 outtime 时间吗？
            ret = pthread_cond_timedwait(&hdl->ack_cond, &hdl->ack_mtx, &outtime);
            if (ret == ETIMEDOUT) {
                printf("0x7fb4cf84 wait time out\n");
                break;
            } else if (ret != 0) {
                printf("0x0a5db736 call fail, ret: %d\n", ret);
            }
        }
    }
    if (ret) {
        pthread_mutex_unlock(&hdl->ack_mtx);
        printf("0x4f885afb run warn, wait ack timeout, ret %d\n", ret);
        return -0x4f885afb;
    }

    /* 取出接收的数据，用 NULL 更新 hash,避免 uc_hash_delete 时一同被删 */
    uc_ipc_proto_packet_t *msg = (uc_ipc_proto_packet_t *)uc_hash_update(hash, key, NULL);
    uc_hash_delete(hash, key);
    pthread_mutex_unlock(&hdl->ack_mtx);

    if (msg) {
        if (free_hdl) {
            *free_hdl = msg;
        }
        // 取出 hash->node->val 中其它的值，因为下面将销毁 hash->node
        if (return_addr) {
            *return_addr = msg->buf;
        }
        if (return_len) {
            *return_len = msg->head.ttl;
        }
    } else {
        printf("0x428f2521 run error, can't find sync-ack data in hash\n");
        return 0x428f2521;
    }

    return 0;
}

int uc_ipc_client_try_send(uc_ipc_client_handler_t *hdl, void *buf, size_t len)
{
    uint64_t msg_id = uc_ipc_client_next_seq_id(hdl);
    int ret = uc_ipc_client_send(hdl, msg_id, uc_ipc_msg_type_void, buf, len);
    if (ret) {
        printf("0x146071f6 insert pool fail, ret: %d", ret);
        return -0x146071f6;
    }

    return ret;
}

int uc_ipc_client_async_send(uc_ipc_client_handler_t *hdl, void *buf, size_t len, int32_t wait_millisecond)
{
    int ret = 0;
    uint64_t seq_id = uc_ipc_client_next_seq_id(hdl);

    /*
     * 先插入再发送，再等待
     * 否则可能发送到等待的这段时间，已经收到了ack(没人认领，导致被误删)，下同
    */
    ret = uc_ipc_client_insert_seq(hdl, seq_id);
    if (ret) {
        printf("-0x3d5ee086 async send fail, ret: %d\n", ret);
        return -0x3d5ee086;
    }

    ret = uc_ipc_client_send(hdl, seq_id, uc_ipc_msg_type_async_send, buf, len);
    if (ret) {
        uc_ipc_client_delete_seq(hdl, seq_id);
        printf("0x274ceea5 ipc client send fail, ret: %d", ret);
        return -0x274ceea5;
    }

    void *ack_msg_hdl = NULL;
    ret = uc_ipc_client_wait_ack(hdl, seq_id, wait_millisecond, &ack_msg_hdl, NULL, NULL);
    if (ret) {
        uc_ipc_client_delete_seq(hdl, seq_id);
        printf("-0x213492c0 wait_ack fail, ret: %d", ret);
        return -0x213492c0;
    }

    free(ack_msg_hdl);
    ack_msg_hdl = NULL;

    return 0;
}

int uc_ipc_client_sync_send(uc_ipc_client_handler_t *hdl, void *buf, size_t len, int32_t wait_millisecond, void **msg_hdl, void **return_addr, size_t *return_len)
{
    *msg_hdl = *return_addr = NULL;
    *return_len = 0;

    int ret = 0;
    uint64_t seq_id = uc_ipc_client_next_seq_id(hdl);

    ret = uc_ipc_client_insert_seq(hdl, seq_id);
    if (ret) {
        printf("-0x7b4123c4 async send fail, ret: %d\n", ret);
        return -0x7b4123c4;
    }

    ret = uc_ipc_client_send(hdl, seq_id, uc_ipc_msg_type_sync_send, buf, len);
    if (ret) {
        uc_ipc_client_delete_seq(hdl, seq_id);
        printf("0x5f879be6 send fail, ret: %d", ret);
        return -0x5f879be6;
    }

    ret = uc_ipc_client_wait_ack(hdl, seq_id, wait_millisecond, msg_hdl, return_addr, return_len);
    if (ret) {
        printf("-0x213492c0 wait_ack fail, ret: %d", ret);
    }
    return ret;
}
