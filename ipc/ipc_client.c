#include "ipc.h"
#include "ipc_common.h"
#include "ipc_io.h"

static void *ipc_client_loop_rcv(void *arg);

static uint64_t ipc_client_next_seq_id(ipc_client_handler_t *hdl)
{
    uint64_t next = 0;
    pthread_mutex_lock(&hdl->mutex);
    next = hdl->seq_id;
    hdl->seq_id++;
    if (hdl->seq_id < IPC_SEQ_ID_MIN) {
        hdl->seq_id = IPC_SEQ_ID_MIN;
    }
    pthread_mutex_unlock(&hdl->mutex);

    return next;
}

/* 插入一个 seq 数据（等待对方 seq 的到来）*/
static int ipc_client_insert_seq(ipc_client_handler_t *hdl, uint64_t seq_id)
{
    int ret = 0;

    pthread_mutex_lock(&hdl->mutex);
    ret = hash_insert(hdl->seq_hash, &seq_id, NULL);
    pthread_mutex_unlock(&hdl->mutex);

    if (ret) {
        printf("0x4cd98a52 insert seq fail, ret: %d", ret);
    }

    return ret;
}

/* 已经收到并处理了对方的 ack(或者ack对应的msg发送失败等原因), 这里删除 ack 相关数据 */
static void ipc_client_delete_seq(ipc_client_handler_t *hdl, uint64_t seq_id)
{
    int ret = 0;

    pthread_mutex_lock(&(hdl->mutex));
    ret = hash_delete(hdl->seq_hash, &seq_id);
    pthread_mutex_unlock(&(hdl->mutex));

    if (ret) {
        printf("error 0x4858115b delete fail, ret: %d", ret);
    }
}

static int ipc_client_send(ipc_client_handler_t *hdl, uint64_t seq_id, ipc_msg_type_t msg_type, void *buf, size_t len)
{
    ipc_proto_t *msg = malloc(sizeof(ipc_proto_t) + len);
    if (NULL == msg) {
        printf("0x1f9385ca malloc fail at ipc client send ...\n");
        return -0x1f9385ca;
    }

    // PROTOCOL 相关代码， 后同
    msg->head.ttl = len;
    msg->head.msg_type = msg_type;
    msg->head.seq_id = seq_id;
    msg->head.ack_id = IPC_ACK_ID_NULL;

    if (len) {
        memcpy(msg->buf, buf, len);
    }

    pthread_rwlock_rdlock(&hdl->rwlock);
    int ret = ipc_send_msg(hdl->sockfd, msg);
    pthread_rwlock_unlock(&hdl->rwlock);
    if (ret) {
        free(msg);
        msg = NULL;
        printf("0x4eb8c9c8 client send fail, ret: %d", ret);
        return ret;
    }

    return ret;
}


void ipc_client_reconnect(ipc_client_handler_t *hdl) {
    while (1) {
        // connect
        printf("connect to server......\n");
        sleep(1);
    }
}


/*
 * 建立于 server 的链接
 * server_ip: 127.0.0.1
 * server_port: 80
 */
ipc_client_handler_t *ipc_init_client(char *server_ip, uint16_t server_port)
{
    ipc_client_handler_t *hdl = calloc(1, sizeof(ipc_client_handler_t));
    if (NULL == hdl) {
        return NULL;
    }

    // fcntl(hdl->pipe[0], F_SETFL, O_NOATIME); /* 提高性能 */
    // fcntl(hdl->pipe[1], F_SETFL, O_NOATIME);

    if (pthread_rwlock_init(&hdl->rwlock, NULL) != 0) {
        free(hdl);
        hdl = NULL;
        return hdl;
    }

    if (pthread_mutex_init(&(hdl->mutex), NULL) != 0) {
        free(hdl);
        hdl = NULL;
        return hdl;
    }
    if (pthread_cond_init(&(hdl->cond), NULL) != 0) {
        pthread_mutex_destroy(&hdl->mutex);
        free(hdl);
        hdl = NULL;
        return hdl;
    }

    strncpy(hdl->server_ip, server_ip, ARRAY_SIZE(hdl->server_ip));
    hdl->server_port = server_port;
    hdl->status = ipc_connect_status_fail;
    hdl->sockfd = -1;
    hdl->thread = NULL;
    hdl->seq_hash = NULL;

    hdl->seq_id = IPC_SEQ_ID_MIN;
    int ret = util_random(&hdl->seq_id, sizeof(hdl->seq_id));
    if (0 != ret) {
        goto err_uninit;
    }
    if (hdl->seq_id < IPC_SEQ_ID_MIN) {
        hdl->seq_id = IPC_SEQ_ID_MIN;
    }

    hdl->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (hdl->sockfd == -1) {
        printf("-0x5e173d0a socket creation failed...\n");
        goto err_uninit;
    }
    ret = setnodelay(hdl->sockfd);    /* ipc 基本用于本机通信，效率为主 */
    if (ret) {
        printf("0x1ec22172 set nodelay fail, ret: %d\n", ret);
    }

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(server_ip);
    servaddr.sin_port = htons(server_port);
    if (connect(hdl->sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        printf("0x603ea840 connection failed...\n");
        hdl->status = ipc_connect_status_fail;
        goto err_uninit;    /* NOTE: 初始化阶段，链接失败直接返回，后续通信过程中，失败可以 retry */
    } else {
        hdl->status = ipc_connect_status_done;
    }

    hdl->seq_hash = hash_create(IPC_CLIENT_SEQ_HASH_SIZE, hash_key_uint64, 0);
    if (NULL == hdl->seq_hash) {
        goto err_uninit;
    }

    hdl->thread = threadpool_create(1, 1, 0);   /* 1 for rcv */
    if (NULL == hdl->thread) {
        goto err_uninit;
    }
    threadpool_add_void_task(hdl->thread, ipc_client_loop_rcv, hdl);
    // 等待 接收线程跑起来
    threadpool_wait_task_done(hdl->thread);

    return hdl;

err_uninit:
    if (NULL != hdl->thread) {
        threadpool_close(hdl->thread);
        hdl->thread = NULL;
    }
    if (NULL != hdl->seq_hash) {
        hash_free(hdl->seq_hash);
        hdl->seq_hash = NULL;
    }
    if (-1 != hdl->sockfd) {
        close(hdl->sockfd);
        hdl->sockfd = -1;
        hdl->status = ipc_connect_status_fail;
    }

    pthread_cond_destroy(&hdl->cond);
    pthread_mutex_destroy(&hdl->mutex);
    pthread_rwlock_destroy(&hdl->rwlock);

    free(hdl);
    hdl = NULL;
    return hdl;
}


// 客户端
static void *ipc_client_loop_rcv(void *arg)
{
    ipc_client_handler_t *hdl = (ipc_client_handler_t *)arg;
    int closed = 0;
    ipc_proto_header_t head;

    while (1) {
        pthread_rwlock_rdlock(&hdl->rwlock);
        int ret = io_read(hdl->sockfd, &head, sizeof(head), &closed);
        pthread_rwlock_unlock(&hdl->rwlock);
        if (ret || closed) {    /* 没读完，报错，或者是被关闭等 */
            // ERROR
            printf("0x7f839461 io.read fail, ret: %d", ret);

            pthread_rwlock_wrlock(&hdl->rwlock);
            close(hdl->sockfd);
            hdl->sockfd = -1;
            hdl->status = ipc_connect_status_fail;
            pthread_rwlock_unlock(&hdl->rwlock);
            ipc_client_reconnect(hdl);  /* reconnect........ */
            continue;
        }
        if (head.ttl < sizeof(head)) {
            // ERROR
        }
        if (head.seq_id < IPC_SEQ_ID_MIN) {
            // ERROR
        }

        ipc_proto_t *msg = (ipc_proto_t*)malloc(sizeof(ipc_proto_header_t) + head.ttl);
        if (NULL == msg) {
            // ERROR
        }
        memcpy(&msg->head, &head, sizeof(head));

        pthread_rwlock_rdlock(&hdl->rwlock);
        ret = io_read(hdl->sockfd, msg->buf, head.ttl, &closed);
        pthread_rwlock_unlock(&hdl->rwlock);
        if (ret || closed) {
            // ERROR
            pthread_rwlock_wrlock(&hdl->rwlock);
            close(hdl->sockfd);
            hdl->sockfd = -1;
            hdl->status = ipc_connect_status_fail;
            pthread_rwlock_unlock(&hdl->rwlock);
            ipc_client_reconnect(hdl);  /* reconnect........ */
            printf("0x17073c93 read fail, ret: %d, closed: %d\n", ret, closed);
        }

        if (msg->head.msg_type == ipc_msg_type_async_ack ||
            msg->head.msg_type == ipc_msg_type_sync_ack) {
            if (head.ack_id < IPC_SEQ_ID_MIN) {
                // ERROR
            }

            pthread_mutex_lock(&hdl->mutex);
            hash_node_t *node = hash_find(hdl->seq_hash, &msg->head.ack_id);
            if (NULL == node) {
                printf("0x025a4313 warn ack too later");
                pthread_cond_signal(&hdl->cond);
                pthread_mutex_unlock(&hdl->mutex);
                free(msg);
                msg = NULL;
            } else {
                // node 的删除，由 wait_ack 处理
                node->val = msg;
                pthread_cond_signal(&hdl->cond);
                pthread_mutex_unlock(&hdl->mutex);
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
static int ipc_client_wait_ack(ipc_client_handler_t *hdl, uint64_t seq_id, int32_t wait_millisecond, void **free_hdl, void **return_addr, size_t *return_len)
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

    int ret = 0;
    pthread_mutex_lock(&hdl->mutex);
    hash_table_t *hash = hdl->seq_hash;
    hash_node_t *node = hash_find(hash, &seq_id);
    if (NULL == node) {
        pthread_mutex_unlock(&hdl->mutex);
        printf("0x790396ad run error, can't find sync-ack data in hash\n");
        return -0x790396ad;
    }

    ret = 0;
    while (NULL == node->val) {
        if (wait_millisecond == 0) {
            ret = pthread_cond_wait(&hdl->cond, &hdl->mutex);
        } else {
            // TODO 需要每次都更新 outtime 时间吗？
            ret = pthread_cond_timedwait(&hdl->cond, &hdl->mutex, &outtime);
            if (ret == ETIMEDOUT) {
                printf("0x7fb4cf84 wait time out\n");
                break;
            } else if (ret != 0) {
                printf("0x0a5db736 call fail, ret: %d\n", ret);
            }
        }
    }
    if (ret) {
        pthread_mutex_unlock(&hdl->mutex);
        printf("0x4f885afb run warn, wait ack timeout, ret \n", ret);
        return -0x4f885afb;
    }

    /* 取出接收的数据，用 NULL 更新 hash,避免 hash_delete 时一同被删 */
    ipc_proto_t *msg = (ipc_proto_t *)hash_update(hash, &seq_id, NULL);
    hash_delete(hash, &seq_id);
    pthread_mutex_unlock(&hdl->mutex);

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

int ipc_client_try_send(ipc_client_handler_t *hdl, void *buf, size_t len)
{
    uint64_t msg_id = ipc_client_next_seq_id(hdl);
    int ret = ipc_client_send(hdl, msg_id, ipc_msg_type_void, buf, len);
    if (ret) {
        printf("0x146071f6 insert pool fail, ret: %d", ret);
        return -0x146071f6;
    }

    return ret;
}

int ipc_client_async_send(ipc_client_handler_t *hdl, void *buf, size_t len, int32_t wait_millisecond)
{
    int ret = 0;
    uint64_t seq_id = ipc_client_next_seq_id(hdl);

    /*
     * 先插入再发送，再等待
     * 否则可能发送到等待的这段时间，已经收到了ack(没人认领，导致被误删)，下同
    */
    ret = ipc_client_insert_seq(hdl, seq_id);
    if (ret) {
        printf("-0x3d5ee086 async send fail, ret: %d\n", ret);
        return -0x3d5ee086;
    }

    ret = ipc_client_send(hdl, seq_id, ipc_msg_type_async_send, buf, len);
    if (ret) {
        ipc_client_delete_seq(hdl, seq_id);
        printf("0x274ceea5 ipc client send fail, ret: %d", ret);
        return -0x274ceea5;
    }

    void *ack_msg_hdl = NULL;
    ret = ipc_client_wait_ack(hdl, seq_id, wait_millisecond, &ack_msg_hdl, NULL, NULL);
    if (ret) {
        ipc_client_delete_seq(hdl, seq_id);
        printf("-0x213492c0 wait_ack fail, ret: %d", ret);
        return -0x213492c0;
    }

    free(ack_msg_hdl);
    ack_msg_hdl = NULL;

    return 0;
}

int ipc_client_sync_send(ipc_client_handler_t *hdl, void *buf, size_t len, int32_t wait_millisecond, void **msg_hdl, void **return_addr, size_t *return_len)
{
    *msg_hdl = *return_addr = NULL;
    *return_len = 0;

    int ret = 0;
    uint64_t seq_id = ipc_client_next_seq_id(hdl);

    ret = ipc_client_insert_seq(hdl, seq_id);
    if (ret) {
        printf("-0x7b4123c4 async send fail, ret: %d\n", ret);
        return -0x7b4123c4;
    }

    ret = ipc_client_send(hdl, seq_id, ipc_msg_type_sync_send, buf, len);
    if (ret) {
        ipc_client_delete_seq(hdl, seq_id);
        printf("0x5f879be6 send fail, ret: %d", ret);
        return -0x5f879be6;
    }

    ret = ipc_client_wait_ack(hdl, seq_id, wait_millisecond, msg_hdl, return_addr, return_len);
    if (ret) {
        printf("-0x213492c0 wait_ack fail, ret: %d", ret);
    }
    return ret;
}
