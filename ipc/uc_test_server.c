#include "uc_ipc.h"
#include "uc_log.h"

/*
 * 10K/s 发送和处理包的能力， packet.size从[1,128K]一次递增
 */

void *callback(uc_ipc_msg_type_t msg_type, void *addr, size_t len, void **return_addr, size_t *return_len)
{
    // static int s_idx = 0;
    static uint64_t rcv_pkt_ttl = 0;

    rcv_pkt_ttl++;
    if (++rcv_pkt_ttl % 10000 == 0) {
        log("0x3e32785f rcv pkt.ttl: %lu, %ld ms\n", rcv_pkt_ttl, uc_time_ms());
    }

    if (msg_type == uc_ipc_msg_type_void) {
        return NULL;
    } else if (msg_type == uc_ipc_msg_type_async_send) {
        return NULL;
    } else if (msg_type == uc_ipc_msg_type_sync_send) {
        // 来什么，复制一份回过去就行
        *return_addr = malloc(len);
        if (*return_addr) {
            memcpy(*return_addr, addr, len);
            *return_len = len;
        } else {
            printf("0x6feef39a run error\n");
        }
    }

    // int idx = 0;
    // memcpy(&idx, addr, len);
    // if(0 == idx % 10000){
    //     printf("0x6bfe2623 rcv len: %d, idx: %d\n", len, idx);
    // }
    // if (len != sizeof(int)) {
    //     printf("0x4e67b44a invalid add: %p, len: %d\n", addr, len);
    // }
    // if (idx == 0) {
    //     s_idx = 0;
    // } else {
    //     s_idx++;
    //     if (idx != s_idx) {
    //         printf("0x0557976c invalid idx, want: %d, get: %d\n", s_idx, idx);
    //     }
    // }
    return NULL;
}


int main(int argc, char *argv[])
{
    uc_ipc_server_handler_t *hdl = uc_ipc_init_server("127.0.0.1", 4000, callback);
    sleep(24*3600);
    return -1;
}
