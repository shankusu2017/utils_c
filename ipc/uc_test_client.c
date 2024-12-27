#include "uc_ipc.h"
#include "uc_random.h"
#include "uc_log.h"


int test_cost(uc_ipc_client_handler_t *hdl)
{
    static int idx = 0;
    int ret = 0;
    size_t times = 65536;
    size_t ttl = times;
    int64_t begainT = uc_time_us();
    while (times--) {
        size_t len = 0;
        uc_random(&len, sizeof(len));
        char *send_buf = (char *)malloc(len);
        if (send_buf) {
            ret = uc_ipc_client_async_send(hdl, send_buf, len, 3000);
            free(send_buf);
            if (ret) {
                return ret;
            }
        }
    }
    int64_t endT = uc_time_us();
    log("%lu async cost: %ldus, avg: %ldus\n", ttl, endT - begainT, (endT- begainT)/256);
    return 0;
}

int test_void(uc_ipc_client_handler_t *hdl, int times)
{
    static int idx = 0;
    int ret = 0;
    while (times--) {
        size_t send_len = sizeof(char) + idx%(1024*256);
        char *send_buf = (char *)malloc(send_len);
        if (send_buf) {
            ++idx;
            if (idx % 10000 == 0) {
                log("0x64c8d752 idx; %ld\n", idx);
            }
            ret = uc_ipc_client_try_send(hdl, send_buf, send_len);
            free(send_buf);
            if (ret) {
                return ret;
            }
        }
    }
    return 0;
}

int test_async(uc_ipc_client_handler_t *hdl, int times) 
{
    static int idx = 0;
    int ret = 0;
    while (times--) {
        size_t send_len = sizeof(char) + idx%(1024*256);
        char *send_buf = (char *)malloc(send_len);
        if (send_buf) {
            idx++;
            ret = uc_ipc_client_async_send(hdl, send_buf, send_len, 0);
            if (idx % 10000 == 0){
                log("async send %d, len: %ld\n", idx, send_len);
            }
            free(send_buf);
            if (ret) {
                return ret;
            }
        }
    }
    return 0;
}

int test_sync(uc_ipc_client_handler_t *hdl, int times) 
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
            if (times % 10000 == 0) {
                log("sync client idx: %d, len: %ld\n", idx, send_len);
            }
            ret = uc_ipc_client_sync_send(hdl, send_buf, send_len, 3*1000, &msg_hdl, &return_addr, &return_len);
            if (ret != 0) {
                log("0x6c523505 test sync fail, ret: %d\n", ret);
                return ret;
            } else {
                if (return_len != send_len) {
                    log("0x12a51788 test sync fail, want.len: %ld, get.len: %ld\n", send_len, return_len);
                    return 0x12a51788;
                }
                if (memcmp(send_buf, return_addr, send_len)) {
                    log("0x51ac087c test sync fail, ret: %d\n", ret);
                    return 0x51ac087c;
                }
                free(send_buf);
                send_buf = NULL;
                free(msg_hdl);
                msg_hdl = NULL;
            }
        } else {
            log("0x34ba7b23 test sync fail");
            return 0x34ba7b23;
        }
    }
    return 0;
}

int test_sync_small_msg(uc_ipc_client_handler_t *hdl, int times, size_t size) 
{
    static int idx = 0;
    int ret = 0;
    void *msg_hdl;
    void *return_addr;
    size_t return_len;
    while (times--) {
        size_t send_len = size;
        char *send_buf = (char *)malloc(send_len);
        if (send_buf) {
            idx++;
            if (times % 10000 == 0) {
                log("sync small client idx: %d, len: %ld\n", idx, send_len);
            }
            ret = uc_ipc_client_sync_send(hdl, send_buf, send_len, 3*1000, &msg_hdl, &return_addr, &return_len);
            if (ret != 0) {
                log("0x211cd456 test sync small fail, ret: %d\n", ret);
                return ret;
            } else {
                if (return_len != send_len) {
                    log("0x46e8f452 test sync small fail, want.len: %ld, get.len: %ld\n", send_len, return_len);
                    return 0x46e8f452;
                }
                if (memcmp(send_buf, return_addr, send_len)) {
                    log("0x083fd58c test sync small fail, ret: %d\n", ret);
                    return 0x083fd58c;
                }
                free(send_buf);
                send_buf = NULL;
                free(msg_hdl);
                msg_hdl = NULL;
            }
        } else {
            printf("0x7a5eb8da test sync fail");
            return 0x7a5eb8da;
        }
    }
    return 0;
}


int main(int argc, char *argv[])
{
    int ret = 0;
    uc_ipc_client_handler_t *hdl = uc_ipc_init_client("127.0.0.1", 4000);
    if (NULL == hdl) {
        log("init client fail\n");
        return -1;
    } else {
        log("init client done\n");
    }

    //test_cost(hdl);
    test_void(hdl, 1024*128);
    //log("0x632adc1e test void done\n");


    ret = test_async(hdl, 1024*128);
    if (ret) {
        log("0x632adc1e test fail, ret: %d\n", ret);
        return -1;
    }
    log("0x632adc1e test async done\n");

    log("0x4366041c sleep 10 for test reconnect\n");
    for (int i = 0; i < 10; i++) {
        sleep(1);
        log("0x527ee2c7 %d for test reconnect\n", 10 - i);
    }
    log("0x0684a5e5 reconnect\n");

    ret = test_sync(hdl, 1024*128);
    if (ret) {
        log("0x50eb3288 test fail, ret: %x\n", ret);
        return -1;
    }
    log("0x0bf6d8a3 test sync done, ret: pass\n");

    ret = test_sync_small_msg(hdl, 1024*128, 64);
    if (ret) {
        log("0x344dc10a test fail, ret: %x\n", ret);
        return -1;
    }
    log("0x714b0881 test sync done, ret: pass\n");

    log("test code run done, test result is ok\n");

    return 0;
}
