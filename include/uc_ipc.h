#ifndef IPC_H_20240820180948_0X2905CDC0
#define IPC_H_20240820180948_0X2905CDC0

/*
 * CLIENT: 主动发送 msg 给 SERVER , 并接收相应的 ack
 * SERVER: 被动接收 CLIENT 的 msg , 调用回调函数(返回ack给对方)
*/

#include "uc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    uc_ipc_msg_type_heartbeat = 1,
    uc_ipc_msg_type_void,
    uc_ipc_msg_type_async_send,
    uc_ipc_msg_type_async_ack,
    uc_ipc_msg_type_sync_send,
    uc_ipc_msg_type_sync_ack,
} uc_ipc_msg_type_t;

typedef void *(*uc_ipc_callback)(uc_ipc_msg_type_t msg_type, void *addr, size_t len, void **return_addr, size_t *return_len);
typedef struct uc_ipc_client_handler_s uc_ipc_client_handler_t;
typedef struct uc_ipc_server_handler_s uc_ipc_server_handler_t;

/*
 * RETURNS : 非空，已经和服务器链接成功，NULL: 链接失败
 */
uc_ipc_client_handler_t *uc_ipc_init_client(char *server_ip, uint16_t server_port);
uc_ipc_server_handler_t *uc_ipc_init_server(char *listen_ip, uint16_t listen_port, uc_ipc_callback cb);
/* 尝试发送，不保证对方收到 */
int uc_ipc_client_try_send(uc_ipc_client_handler_t *hdl, void *buf, size_t len);
/* 发送，对方返回 ack */
int uc_ipc_client_async_send(uc_ipc_client_handler_t *hdl, void *buf, size_t len, int32_t wait_millisecond);
/* 发送，并接收对方的返回数据 */
int uc_ipc_client_sync_send(uc_ipc_client_handler_t *hdl, void *buf, size_t len, int32_t wait_millisecond, void **msg_hdl, void **return_addr, size_t *return_len);

#ifdef __cplusplus
}
#endif

#endif /* IPC_H_20240820180948_0X2905CDC0 */
