#ifndef IPC_IO_H_20240821105048_0X4EF015E8
#define IPC_IO_H_20240821105048_0X4EF015E8

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int io_read(int fd, void *buf, size_t len, int *closed);
extern int io_write(int fd, void *buf, size_t len);

extern void *ipc_send_msg(int fd, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* IPC_IO_H_20240821105048_0X4EF015E8 */
