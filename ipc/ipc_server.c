#include "ipc.h"
#include "ipc_common.h"
#include "ipc_io.h"
#include "random.h"
#include <fcntl.h>

static void *uc_ipc_server_loop_rcv(void *arg);
static int uc_ipc_server_accept_client(uc_ipc_server_handler_t *hdl, uc_ipc_cli_t *cli);
static int uc_ipc_server_read_client(uc_ipc_server_handler_t *hdl, uc_ipc_cli_t *cli);
static int uc_ipc_server_handle_client_msg(uc_ipc_server_handler_t *hdl, uc_ipc_cli_t *cli);
static void *uc_ipc_server_loop_callback(void *arg);
static int uc_ipc_server_send_msg(uc_ipc_server_handler_t *hdl, uint64_t cli_uuid, uint64_t ack_id, uc_ipc_msg_type_t msg_type, void *buf, size_t len);
static void uc_ipc_server_free_pipe_msg(uc_ipc_pipe_data_t *msg);

static uint64_t uc_ipc_server_next_seq_id(uc_ipc_server_handler_t *hdl)
{
    uint64_t next = 0;
    pthread_mutex_lock(&hdl->seq_mutex);
    next = hdl->seq_id;
    hdl->seq_id++;
    if (hdl->seq_id < UC_IPC_SEQ_ID_MIN) {
        hdl->seq_id = UC_IPC_SEQ_ID_MIN;
    }
    pthread_mutex_unlock(&hdl->seq_mutex);

    return next;
}

static int uc_ipc_server_send_ack(uc_ipc_server_handler_t *hdl, uint64_t cli_uuid, uint64_t ack_id)
{
    int ret = uc_ipc_server_send_msg(hdl, cli_uuid, ack_id, uc_ipc_msg_type_async_ack, NULL, 0);
    if (ret) {
        printf("0x274ceea5 send pool fail, ret: %d", ret);
        return -0x274ceea5;
    }

    return ret;
}

static void uc_ipc_server_reset_cli_proto(uc_ipc_cli_t *cli)
{
    cli->has_read = cli->head_done = cli->buf_done = 0;
    memset(&cli->ipc_head, 0, sizeof(cli->ipc_head));

    /* 关闭 client 时，要 free 
     * 准备读下一份 msg 时，本份数据的 buf 已经传给 pipe，且置为 NULL 了 
     * 故而可以在这里调用 free
     */
    free(cli->buf);
    cli->buf = NULL;
}

static void uc_ipc_server_close_cli(uc_ipc_server_handler_t *hdl, uc_ipc_cli_t *cli)
{
    uint64_t uuid = cli->uuid;

    uc_hash_key_t key = {.u64 = uuid};

    printf("0x085a33b8 close client, uuid: %lx\n", uuid);

    /* 解除 uuid->fd 的映射*/
    pthread_mutex_lock(&hdl->cli_fd_mutex);
    uc_hash_delete(hdl->cli_fd_hash, key);
    pthread_mutex_unlock(&hdl->cli_fd_mutex);

    int fd = cli->fd;

    /* A: 先清理 cli */

    /* 先处理协议域 */
    uc_ipc_server_reset_cli_proto(cli);
    /* 再处理基本信息 */
    close(cli->fd);
    cli->fd = -1;
    memset(&cli->addr, 0, sizeof(cli->addr));
    cli->uuid = 0;

    /* B: 再 处理 hash */
    uc_hash_delete(hdl->cli_info_hash, key); // cli 无效了
    cli = NULL;

    /* 处理 epoll */
    epoll_ctl(hdl->epoll, EPOLL_CTL_DEL, fd, NULL);
}


uc_ipc_server_handler_t *uc_ipc_init_server(char *listen_ip, uint16_t listen_port, uc_ipc_callback cb)
{
    uc_ipc_server_handler_t *hdl = calloc(1, sizeof(uc_ipc_server_handler_t));
    if (NULL == hdl) {
        return NULL;
    }
    hdl->sockfd = -1;
    hdl->epoll = -1;
    hdl->pipe[0] = -1;
    hdl->pipe[1] = -1;
    hdl->cb = cb;

    //int (pthread_rwlock_t *restrict rwlock,const pthread_rwlockattr_t *restrict attr);
    if (pthread_mutex_init(&hdl->cli_fd_mutex, NULL) != 0) {
        printf("0x3b883dc4 init fail");
        free(hdl);
        hdl = NULL;
        return hdl;
    }
    if (pthread_mutex_init(&hdl->seq_mutex, NULL)) {
        printf("0x620d7bbc init fail");
        free(hdl);
        hdl = NULL;
        return hdl;
    }

    hdl->seq_id = UC_IPC_SEQ_ID_MIN;
    int ret = uc_random(&hdl->seq_id, sizeof(hdl->seq_id));
    if (0 != ret) {
        goto err_uninit;
    }
    if (hdl->seq_id < UC_IPC_SEQ_ID_MIN) {
        hdl->seq_id = UC_IPC_SEQ_ID_MIN;
    }

    /* 将收到的信息传给专门处理 callback 的线程 */
    // PIPE 的 BUF 大一些为好
    ret = pipe(hdl->pipe);
    if (ret) {
        printf("0x00585c77 init fail");
        goto err_uninit;
    }
    // fcntl(hdl->pipe[0], F_SETFL, O_NOATIME); /* 提高性能 */
    // fcntl(hdl->pipe[1], F_SETFL, O_NOATIME);

    hdl->epoll = epoll_create(16*1024);
	if (hdl->epoll == -1) {
        printf("0x285ab164 epoll init fail");
        goto err_uninit;
	}

    hdl->cli_info_hash = uc_hash_create(UC_IPC_SERVER_CLIENT_MAP_SIZE, uc_hash_key_uint64, 0);
    if (NULL == hdl->cli_info_hash) {
        printf("0x285ab171 cli.info.hash init fail");
        goto err_uninit;
    }

    hdl->cli_fd_hash = uc_hash_create(UC_IPC_SERVER_CLIENT_MAP_SIZE, uc_hash_key_uint64, 0);
    if (NULL == hdl->cli_fd_hash) {
        printf("0x5ad92d78 cli.fd.hash init fail");
        goto err_uninit;
    }

    hdl->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == hdl->sockfd) {
        printf("0x285ab517 socket creation failed...\n");
        goto err_uninit;
    }
    uc_setnodelay(hdl->sockfd);
    uc_setreuse(hdl->sockfd);
    uc_setnonblocking(hdl->sockfd);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(uc_utils_ipv4_str_to_host(listen_ip));
    servaddr.sin_port = htons(listen_port);

    // Binding newly created socket to given IP and verification
    if (bind(hdl->sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        printf("0x4d7f3595 socket bind failed...\n");
        goto err_uninit;
    }

    if (listen(hdl->sockfd, 1024) != 0) {
        printf("0x0629237e Listen failed...\n");
        goto err_uninit;
    }
    hdl->sock_cli = (uc_ipc_cli_t*)calloc(1, sizeof(uc_ipc_cli_t));
    if (NULL == hdl->sock_cli) {
        goto err_uninit;
    }
    hdl->sock_cli->fd = hdl->sockfd;

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLERR;
    ev.data.ptr = hdl->sock_cli;
    ret = epoll_ctl(hdl->epoll, EPOLL_CTL_ADD, hdl->sockfd, &ev);
    if (ret == -1) {
        printf("0x4acf7c01 add listen to epoll fail");
        goto err_uninit;
    }

    hdl->thread = uc_threadpool_create(2, 2, 0);   /* rcv + cb */
    if (NULL == hdl->thread) {
        goto err_uninit;
    }
    uc_threadpool_add_void_task(hdl->thread, uc_ipc_server_loop_rcv, hdl);
    uc_threadpool_add_void_task(hdl->thread, uc_ipc_server_loop_callback, hdl);
    // 等待 接收、回调线程跑起来
    uc_threadpool_wait_task_done(hdl->thread);

    return hdl;

err_uninit:
    if (hdl->thread) {
        uc_threadpool_close(hdl->thread);
        hdl->thread = NULL;
    }
    if (hdl->sock_cli) {
        free(hdl->sock_cli);
        hdl->sock_cli = NULL;
    }
    if (-1 != hdl->sockfd) {
        close(hdl->sockfd);
        hdl->sockfd = -1;
    }
    if (hdl->cli_fd_hash) {
        uc_hash_free(hdl->cli_fd_hash);
        hdl->cli_fd_hash = NULL;
    }
    if (hdl->cli_info_hash) {
        uc_hash_free(hdl->cli_info_hash);
        hdl->cli_info_hash = NULL;
    }
    if (hdl->epoll != -1) {
        close(hdl->epoll);
        hdl->epoll = -1;
    }
    if (hdl->pipe[0] != -1) {
        close(hdl->pipe[0]);
        hdl->pipe[0] = -1;
    }
    if (hdl->pipe[1] != -1) {
        close(hdl->pipe[1]);
        hdl->pipe[1] = -1;
    }
    hdl->cb = NULL;

    pthread_mutex_destroy(&hdl->cli_fd_mutex);
    pthread_mutex_destroy(&hdl->seq_mutex);

    free(hdl);
    hdl = NULL;
    return hdl;
}


static void *uc_ipc_server_loop_rcv(void *arg)
{
    uc_ipc_server_handler_t *hdl = (uc_ipc_server_handler_t *)arg;

    while (1) {
        struct epoll_event evs[128];
        int64_t debug_select_start_time = uc_time_ms();
        int num = epoll_wait(hdl->epoll, evs, ARRAY_SIZE(evs), -1);
        int64_t debug_select_end_time = uc_time_ms();
        if (debug_select_end_time - debug_select_start_time > 10) {
            printf("0x451b00d0 select wait too long %ldms------------------\n", debug_select_end_time - debug_select_start_time);
        }
        for (int i = 0; i < num; ++i) {
            void *ptr = evs[i].data.ptr;
            if (ptr == hdl->sock_cli) {
                int ret = uc_ipc_server_accept_client(hdl, (uc_ipc_cli_t *)ptr);
                if (ret) {
                    printf("0x59f07af6 accept fail, %d", ret);
                }
            } else {
                int ret = uc_ipc_server_read_client(hdl, (uc_ipc_cli_t *)ptr);
                if (ret < 0) {
                    printf("0x0980bb38 rcv ret:%x\n", ret);
                } else if(ret == 0) {
                    continue;	// go on read
                } else if (ret > 0) {
                    ret = uc_ipc_server_handle_client_msg(hdl, (uc_ipc_cli_t *)ptr);
                    if (ret) {
                        printf("0x4eca6368 run error:%d\n", ret);
                    }
                }
            }
        } 
        if (num < 0) {
            printf("0x7a1c7608 epool wait fail, ret: %d\n", num);
        }
    }

    return NULL;
}

// TODO 更详细的错误处理
static int uc_ipc_server_accept_client(uc_ipc_server_handler_t *hdl, uc_ipc_cli_t *listen_cli)
{
    uc_ipc_cli_t *cli = NULL;
    int ret = 0;
    struct sockaddr_in cliAddr;
    int len = sizeof(cliAddr);
    int fd = accept(listen_cli->fd, (struct sockaddr *)&cliAddr, &len);
    if (fd < 0) { // ERROR
        printf("0x4f5d7da6 server accept failed...\n");
        return -0x4f5d7da6;
    }
    printf("server accept the client...\n");
    uc_setnonblocking(fd);
    uc_setnodelay(fd);
    uc_setrcvbuf(fd, UC_IPC_SOCK_BUF_LEN);

    cli = calloc(1, sizeof(uc_ipc_cli_t));
    if (NULL == cli) {
        close(fd);
        printf("0x1d008be0 calloc fail for accept\n");
        return -0x1d008be0;
    }
    cli->fd = fd;
    memcpy(&cli->addr, &cliAddr, len);
    ret = uc_urandom(&cli->uuid, sizeof(cli->uuid));
    if (ret) {
        printf("0x363246db get uuid fail, fd: %d\n", fd);
        cli->uuid = ((uint32_t)cliAddr.sin_addr.s_addr) << 16 + cliAddr.sin_port;
    }
    /* 其它字段自动被设为 0（上面的calloc）*/

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLERR;	/* 检测 fd 读缓冲区是否有数据 */
    ev.data.ptr = cli;
    ret = epoll_ctl(hdl->epoll, EPOLL_CTL_ADD, fd, &ev);
    if (ret == -1) {
        close(fd);
        free(cli);
        cli = NULL;
        printf("0x59f07af6 add to epoll fail, fd: %d\n", fd);
        return -0x59f07af6;
    }

    uc_hash_key_t key = {.u64 = cli->uuid};
    int *fd_mem = (int *)malloc(sizeof(fd));
    if (fd_mem) {
        *fd_mem = fd;
        pthread_mutex_lock(&hdl->cli_fd_mutex);
        ret = uc_hash_insert(hdl->cli_fd_hash, key, fd_mem);
        pthread_mutex_unlock(&hdl->cli_fd_mutex);
        if (ret) {
            printf("0x4b68c8d0 insert cli.fd.hash fail, ret:%d", ret);
        }
    } else {
        close(fd);
        free(cli);
        cli = NULL;
        printf("0x70a1fdb5 malloc fail for fd_mem, %d", fd);
        return -0x70a1fdb5;
    }

    ret = uc_hash_insert(hdl->cli_info_hash, key, cli);
    if (ret) {
        printf("0x59b70e22 insert cli.info.hash fail, ret:%d", ret);
        return -0x59b70e22;
    }

    return 0;
}


static int uc_ipc_server_read_client(uc_ipc_server_handler_t *hdl, uc_ipc_cli_t *cli)
{
    while (1) {
        size_t want = 0;
        int count = 0;

        /* 先读协议头，再读 buf */
        if (0 == cli->head_done) {
            want = sizeof(cli->ipc_head) - cli->has_read;
            count = read(cli->fd, (char *)&cli->ipc_head + cli->has_read, want);
        } else if (0 == cli->buf_done) {
            want = cli->ipc_head.ttl - (cli->has_read - sizeof(cli->ipc_head));
            if (want > UC_IPC_MSG_READ_BLOCK_LEN_MAX) {
                want = UC_IPC_MSG_READ_BLOCK_LEN_MAX;
            }
            count = read(cli->fd, (char *)cli->buf + (cli->has_read - sizeof(cli->ipc_head)), want);
        } else {
            uc_ipc_server_close_cli(hdl, cli);
            printf("0x34a82f5c run error\n");   // ERROR
            return -0x34a82f5c;
        }
        if (want != count && count < 65451) {
            //printf("0x474ea942  want: %ld, count: %d\n", want, count);
        }
        if (count > 0) {
            cli->has_read += count;
            if (cli->head_done == 0) {
                // 协议头读取完毕
                if (cli->has_read == sizeof(cli->ipc_head)) {
                    cli->head_done = 1;
                    if (cli->has_read == sizeof(cli->ipc_head) + cli->ipc_head.ttl) {
                        cli->buf = NULL;
                        cli->buf_done = 1;  // 无 buf 需要读
                    } else {
                        cli->buf = malloc(cli->ipc_head.ttl);
                        if (NULL == cli->buf) {
                            uc_ipc_server_close_cli(hdl, cli);
                            printf("0x48a6af21 realloc for rcv msg fail, realloc.mem.len: %lu\n", cli->ipc_head.ttl);
                            return -0x48a6af21;
                        }
                    }
                } else if (cli->has_read > sizeof(cli->ipc_head)) {
                    uc_ipc_server_close_cli(hdl, cli);
                    printf("0x4a9e0f3d run error, %lu\n", cli->has_read);
                    return -0x4a9e0f3d;
                } else {
                    printf("0x5e325782 pleast wait, want: %ld, count: %d\n", want, count);
                    // go on
                    return 0;
                }
            } else if (cli->buf_done == 0) {
                if (cli->has_read == sizeof(cli->ipc_head) + cli->ipc_head.ttl) {
                    cli->buf_done = 1;
                } else if (cli->has_read > sizeof(cli->ipc_head) + cli->ipc_head.ttl) {
                    uc_ipc_server_close_cli(hdl, cli);
                    printf("0x73978bc5 run error, %lu:%lu\n", cli->has_read, cli->ipc_head.ttl);
                    return -0x73978bc5;
                }else {
                    //if (count < 65451) {
                    //    printf("0x321d0ad0 pleast wait, want: %ld, count: %d\n", want, count);
                    //}
                    // go on
                    return 0;
                }
            }

            // 读完了一条消息
            if (cli->buf_done != 0) {
                return 1;
            }
            if (count < want) {
                printf("0x7d879d72 pleast wait, want: %ld, count: %d\n", want, count);
                return 0;   // 尚有部分要求读的数据未到，有数据后，再继续读
            } else {
                continue;   /* 读完 head, 接着读 buf */
            }
        } else if (count == 0) {    /* fd closed */
            uc_ipc_server_close_cli(hdl, cli);
            printf("0x46dd3fd2 client closed, errno: %d\n", errno);
            return -0x46dd3fd2;
        } else {
            if (errno == EINTR) {
                printf("0x768ab597 EINTR\n");
                continue;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {    /* 对于 noblock 而言，无数据可读 */
                // TODOD
                printf("0x75b42200  read fail, EAGAIN or EWOULDBLOCK\n");
                return 0;
            } else {
                uc_ipc_server_close_cli(hdl, cli);
                printf("0x31ecfc83 read fail, errno: %d\n", errno);
                return -0x31ecfc83;
            }
        }
    }
}



static int uc_ipc_server_handle_client_msg(uc_ipc_server_handler_t *hdl, uc_ipc_cli_t *cli)
{
    if (cli->buf_done == 0) {
        printf("0x33a51be5 run error\n");
        return -0x33a51be5;
    }

    uc_ipc_pipe_data_t *pipe_msg = malloc(sizeof(uc_ipc_pipe_data_t));
    if (NULL == pipe_msg) {
        printf("0x19171e01 malloc fail\n");
        uc_ipc_server_reset_cli_proto(cli);
        return -0x19171e01;
    }

    pipe_msg->cli_uuid = cli->uuid;
    memcpy(&pipe_msg->ipc_head, &cli->ipc_head, sizeof(pipe_msg->ipc_head));
    pipe_msg->buf = cli->buf;
    cli->buf = NULL;    /* 所属 buf 已经移交 给 pipe */
    pipe_msg->return_addr = NULL;
    pipe_msg->return_len = 0;

    /* 重置 cli, 为读下一份 msg 做准备 */
    uc_ipc_server_reset_cli_proto(cli);

    if (pipe_msg->ipc_head.msg_type == uc_ipc_msg_type_void) {
        #ifdef IPC_DEBUG_IO
            static size_t rcv_ttl = 0;
            if (++rcv_ttl % 10000 == 0) {
                log("0x1958b03e rcv ttl: %ld", rcv_ttl);
            }
            uc_ipc_server_free_pipe_msg(pipe_msg);
            return 0;
        #endif

        // callback
        int ret = uc_io_write(hdl->pipe[1], &pipe_msg, sizeof(pipe_msg));
        if (ret) {
            uc_ipc_server_free_pipe_msg(pipe_msg);
            printf("0x457e761d write to pipe fail, ret:%d\n", ret);
            return -0x457e761d;
        }
    } else if (pipe_msg->ipc_head.msg_type == uc_ipc_msg_type_async_send) {
        // 立刻发送 ack 给对方
        uc_ipc_server_send_ack(hdl, cli->uuid, pipe_msg->ipc_head.seq_id);
        // callback
        int ret = uc_io_write(hdl->pipe[1], &pipe_msg, sizeof(pipe_msg));
        if (ret) {
            uc_ipc_server_free_pipe_msg(pipe_msg);
            printf("0x1c15b0bb write to pipe fail, ret:%d\n", ret);
        }
    } else if (pipe_msg->ipc_head.msg_type == uc_ipc_msg_type_sync_send) {
        // callback，并且由callback函数返回 ack 给 client
        int ret = uc_io_write(hdl->pipe[1], &pipe_msg, sizeof(pipe_msg));
        if (ret) {
            uc_ipc_server_free_pipe_msg(pipe_msg);
            printf("0x5afb2312 write to pipe fail, ret:%d\n", ret);
            return -0x5afb2312;
        }
    } else {    /* 收到了不应该收到的信息类型 */
        uc_ipc_server_free_pipe_msg(pipe_msg);
        printf("0x0421a96f WARN rcv invalid msg(%d)\n", pipe_msg->ipc_head.msg_type);
        return -0x0421a96f;
    }

    return 0;
}


static void uc_ipc_server_free_pipe_msg(uc_ipc_pipe_data_t *pipe_msg)
{
    pipe_msg->cli_uuid = 0;
    free(pipe_msg->buf);
    pipe_msg->buf = NULL;

    free(pipe_msg->return_addr);
    pipe_msg->return_addr = NULL;
    pipe_msg->return_len = 0;

    free(pipe_msg);
    // pipe_msg = NULL;
}

static void *uc_ipc_server_loop_callback(void *arg)
{
    uc_ipc_server_handler_t *hdl = (uc_ipc_server_handler_t *)arg;
    uc_ipc_pipe_data_t *pipe_msg;
    int closed;

    while (1)  {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(hdl->pipe[0], &fds);
        struct timeval timeout = {10, 0};
        int ret = select(hdl->pipe[0] + 1, &fds, NULL, NULL, &timeout);
        if (ret < 0) {
            printf("select error\n");
            break;
        }  else if (ret == 0) {
            // printf("no fd ready\n");
            continue;
        }  else {
            if (FD_ISSET(hdl->pipe[0], &fds) > 0) {
                ret = uc_io_read(hdl->pipe[0], &pipe_msg, sizeof(pipe_msg), &closed);
                if (ret) {
                    printf("0x6cc13f4c io.read fail ret: %d\n", ret);
                    continue;
					// TODO  进一步的处理
                    // read error
                }
                if (pipe_msg->ipc_head.msg_type == uc_ipc_msg_type_void ||
                    pipe_msg->ipc_head.msg_type == uc_ipc_msg_type_async_send) {
                    hdl->cb(pipe_msg->ipc_head.msg_type, pipe_msg->buf, pipe_msg->ipc_head.ttl, &pipe_msg->return_addr, &pipe_msg->return_len);
                } else if (pipe_msg->ipc_head.msg_type == uc_ipc_msg_type_sync_send) {
                    hdl->cb(pipe_msg->ipc_head.msg_type, pipe_msg->buf, pipe_msg->ipc_head.ttl, &pipe_msg->return_addr, &pipe_msg->return_len);
                    uc_ipc_server_send_msg(hdl, pipe_msg->cli_uuid, pipe_msg->ipc_head.seq_id, uc_ipc_msg_type_sync_ack, pipe_msg->return_addr, pipe_msg->return_len);
                }
                // TODO 回消息给 client
                //TODO
                // 处理下 msg 对应的内存
                // 根据 msg_type 来做进一步的处理
                uc_ipc_server_free_pipe_msg(pipe_msg);
                pipe_msg = NULL;
            }
        }
    }
}


static int uc_ipc_server_send_msg(uc_ipc_server_handler_t *hdl, uint64_t cli_uuid, uint64_t ack_id, uc_ipc_msg_type_t msg_type, void *buf, size_t len)
{
    uint64_t seq_id = uc_ipc_server_next_seq_id(hdl);
	int fd = -1;

    uc_ipc_proto_packet_t *data = malloc(sizeof(uc_ipc_proto_header_t) + len);
    if (NULL == data) {
        printf("0x1f9385ca malloc fail...\n");
        return -0x1f9385ca;
    }

    data->head.ttl = len;
    data->head.msg_type = msg_type;
    data->head.seq_id = seq_id;
    data->head.ack_id = ack_id;

    if (len) {
        memcpy(data->buf, buf, len);
    }

    uc_hash_key_t key = {.u64 = cli_uuid};

    pthread_mutex_lock(&hdl->cli_fd_mutex);
    uc_hash_node_t *node = uc_hash_find(hdl->cli_fd_hash, key);
    if (node) {
        fd = *(int *)uc_hash_node_val(node);
    } else { /* 从收到客户端消息到调用回调函数处理完毕，再将结果发回客户端的这个过程中，
              * 客户端可能已经出错等原因，已被删除，原先的 client_fd 已失效
              * client fd 可能被用到新的链接上来的 client 上
              * 故而不能用 fd ，而是要根据 client_uuid, 实时的获取client当前的fd的值
              */
        pthread_mutex_unlock(&hdl->cli_fd_mutex);
        free(data);
        data = NULL;
        printf("0x5e8ef49f can't find cli: %lu\n", cli_uuid);
        return -0x5e8ef49f;
    }
    if (fd < 0) {
        // TODO 
    }

    int ret = uc_io_write(fd, data, sizeof(data->head) + data->head.ttl);
    pthread_mutex_unlock(&hdl->cli_fd_mutex);
    free(data);
	data = NULL;
    if (ret) {
        printf("0x4eb8c9c8 add task fail, ret: %d", ret);
        return ret;
    }

    return ret;
}
