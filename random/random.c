#include "common.h"
#include "random.h"

int util_random(void *buf, size_t len)
{
    memset(buf, 0, len);

    int fd = open("/dev/random", O_RDONLY);
    if (fd == -1) {
        return -0x0fe3a13d;
    }
 
    size_t left = len;
    size_t ttl = 0;
    while (left > 0) {
        int ret = read(fd, buf + ttl, left);
        if (ret > 0) {
            left -= ret;
            ttl += ret;
        } else if (ret == 0) {
            continue;
        } else {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                break;
            }
        }
    }
    close(fd);

    if (left > 0) {
        return -0x5d4a2a08;
    } else {
        return 0;
    }
}

