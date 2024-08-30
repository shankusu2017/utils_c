#include "ipc.h"

static void pipesig_handler(int)
{
    return;
}

static int test_sync_send_heartbeat(ipc_client_handler_t *hdl)
{
    int ret = 0;
    int send_len = 2048;
    void *msg_hdl;
    void *return_addr;
    size_t return_len;

    char *send_buf = (char *)calloc(1, send_len);
    if (NULL == send_buf) {
        printf("0x0c41d234 malloc fail\n");
        return -0x0c41d234;
    }

    ret = ipc_client_sync_send(hdl, send_buf, send_len, 3*1000, &msg_hdl, &return_addr, &return_len);
    //printf("0x1b5223cf send hearbeat done %d\n", send_len);
    free(send_buf);
    if (ret) {
        return ret;
    }
    free(msg_hdl);

    return 0;
}


int test_cost(ipc_client_handler_t *hdl)
{
    static int idx = 0;
    int ret = 0;
    size_t times = 256;
    int64_t begainT = utils_us();
    while (times--) {
        size_t len = 0;
        util_random(&len, sizeof(len));
        char *send_buf = (char *)malloc(len);
        if (send_buf) {
            ret = ipc_client_async_send(hdl, send_buf, send_len);
            free(send_buf);
            if (ret) {
                return ret;
            }
        }
    }
    int64_t endT = utils_us();
    printf("%d async cost: %dus, avg: %dus\n", endT - begainT, (endT- begainT)/256);
    return 0;
}

int test_void(ipc_client_handler_t *hdl, int times)
{
    static int idx = 0;
    int ret = 0;
    while (times--) {
        size_t send_len = sizeof(char) + idx%(1024*256);
        //size_t send_len = 65483;
        char *send_buf = (char *)malloc(send_len);
        if (send_buf) {
            ++idx;
            // for (int i =0; i < send_len; ++i) {
            //     send_buf[i] = (char)i;
            // }
            ret = ipc_client_try_send(hdl, send_buf, send_len);
            // if (idx % 1000 == 0){
                printf("void send %d, %d\n", idx, send_len);
            // }
            free(send_buf);
            if (ret) {
                return ret;
            }
        }
    }
    return 0;
}

int test_async(ipc_client_handler_t *hdl, int times) 
{
    static int idx = 0;
    int ret = 0;
    while (times--) {
        size_t send_len = sizeof(char) + idx%(1024*256);
        //size_t send_len = 65483;
        char *send_buf = (char *)malloc(send_len);
        if (send_buf) {
            idx++;
            // for (int i =0; i < send_len; ++i) {
            //     send_buf[i] = (char)i;
            // }
            //ret = ipc_client_async_send(hdl, send_buf, send_len, 3*1000);
            ret = ipc_client_async_send(hdl, send_buf, send_len, 0);
            // if (idx % 1000 == 0){
                printf("async send %d, len: %d\n", idx, send_len);
            // }
            free(send_buf);
            if (ret) {
                return ret;
            }
        }
    }
    return 0;
}

int test_sync(ipc_client_handler_t *hdl, int times) 
{
    static int idx = 0;
    int ret = 0;
    void *msg_hdl;
    void *return_addr;
    size_t return_len;
    while (times--) {
        size_t send_len = sizeof(char) + idx%(1024*256);
        char *send_buf = (char *)malloc(send_len);
        if (send_buf) {
            idx++;
            // for (int i =0; i < send_len; ++i) {
            //     send_buf[i] = (char)i;
            // }
            // if (times % 1000 == 0) {
                printf("sync client idx: %d, len: %d\n", idx, send_len);
            // }
            //test_sync_send_heartbeat(hdl);
            ret = ipc_client_sync_send(hdl, send_buf, send_len, 3*1000, &msg_hdl, &return_addr, &return_len);
            // if (idx % 1024 == 0){
            //     printf("0x7741b00f send %d\n", idx);
            //     return ret;
            // }
            if (ret != 0) {
                printf("0x6c523505 test sync fail, ret: %d\n", ret);
                return ret;
            } else {
                if (return_len != send_len) {
                    printf("0x12a51788 test sync fail, want.len: %d, get.len: %d\n", send_len, return_len);
                    return 0x12a51788;
                }
                if (memcmp(send_buf, return_addr, send_len)) {
                    printf("0x51ac087c test sync fail, ret: %d\n", ret);
                    return 0x51ac087c;
                }
                free(send_buf);
                send_buf = NULL;
                free(msg_hdl);
                msg_hdl = NULL;
            }
        } else {
            printf("0x34ba7b23 test sync fail");
            return 0x34ba7b23;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    //signal(SIGPIPE, SIG_IGN);
    signal(SIGPIPE, pipesig_handler);
    ipc_client_handler_t *hdl = ipc_init_client("127.0.0.1", 4000);
    if (NULL == hdl) {
        printf("init client fail\n");
        return -1;
    }

    test_cost(hdl);

    test_void(hdl, 1024*128);
    printf("0x632adc1e test void done\n");


    ret = test_async(hdl, 1024*128);
    if (ret) {
        printf("0x632adc1e test fail, ret: %d\n", ret);
        return -1;
    }
    printf("0x632adc1e test async done\n");

    ret = test_sync(hdl, 1024*128);
    if (ret) {
        printf("0x50eb3288 test fail, ret: %x\n", ret);
        return -1;
    }
    printf("0x50eb3288 test sync done, ret: pass\n");

    printf("test code run done, test result is ok\n");

    return 0;
}
