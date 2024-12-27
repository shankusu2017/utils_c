#include "uc_ipc.h"
#include "uc_ipc_io.h"
#include "uc_ipc_common.h"

// RETURNS: 负数，读失败, OTHRES: 剩余读取数(0:读到了指定数量的数据)
// closed: 0 接口未关闭， 1: 读的过程中，接口被关闭（对方关闭了 sock 等)
int uc_io_read(int fd, void *buf, size_t len, int *closed)
{
    if (closed) {
        *closed = 0;
    }

    size_t ttl = 0;
    size_t left = len;
    while (left > 0) {
        int ret = read(fd, (char *)buf + ttl, left);
        if (ret > 0) {
            left -= ret;
            ttl += ret;

            if (0 == left) {
                break;  /* 已读完指定数量的数据 */
            }
        } else if (ret == 0) {  /* 文件尾或者对方关闭了 socket */
            if (closed) {
                *closed = 1;
            }
            break;
        } else {
            // The call was interrupted by a signal before any data was read
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 对于 noblock 而言，无数据可读
                // block 而言则不会报此错误码
                break;
            } else {
                printf("0x09ae5c0e read fail errno: %d, len: %lu, left: %lu\n",
                        errno, ttl, left);
				return -0x09ae5c0e;
                //break;
            }
        }
    }

    return left;
}

// RETURNS: 负数，写失败, OTHRES: 剩余待写入数(0:写了指定数量的数据)
int uc_io_write(int fd, void *buf, size_t left)
{
    size_t ttl = 0;
    while (left > 0) {
        int ret = write(fd, (char *)buf + ttl, left);
        if (ret > 0) {
            left -= ret;
            ttl += ret;

            if (0 == left) {    // 数据写入完毕
                break;
            }
        } else if (ret == 0) {
            printf("0x7e59a65f write done, ret 0\n");    // 兼容 left == 0
            break;
        } else {
            if (errno == EINTR){
                continue;
            } else if (errno == EAGAIN) { // TODO EWOULDBLOCK
                printf("0x5cfd165d errno: %d\n", errno);
                continue;   // 即时没有 lefe-space 也一直写，直到写完为止
             } else {
                // ERROR
                printf("0x11b0ae4f fd: %d, ret: %d, errno: %d\n", fd, ret, errno);
                return -0x11b0ae4f;
            }
        }
    }

    return left;
}
