#ifndef IPC_IO_H_20240821105048_0X4EF015E8
#define IPC_IO_H_20240821105048_0X4EF015E8

#include "uc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int uc_io_read(int fd, void *buf, size_t len, int *closed);
extern int uc_io_write(int fd, void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* IPC_IO_H_20240821105048_0X4EF015E8 */
