#ifndef COMMON_H_20240820181327_0X0CADDBF7
#define COMMON_H_20240820181327_0X0CADDBF7

/* 内部用 */
#ifndef _WIN32
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/select.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <unistd.h> // read(), write(), close()
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>          /* See NOTES */
#include <signal.h>



#define MAC_LEN_BYTES (6)
#define MAC_LEN_STR (17)
/* "ff:ee:dd:cc:bb:aa" 包含尾部的\0 在内*/
#define MAC_LEN_STR_00 (MAC_LEN_STR+1)

/* 192.168.251.165 */
#define IPV4_LEN_STR 15
#define IPV4_LEN_STR_00 (IPV4_LEN_STR+1)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE( ARRAY ) (sizeof (ARRAY) / sizeof (ARRAY[0]))
#endif

#ifndef UNUSED
#define UNUSED(x)	(void)(x)
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


extern unsigned host_count(unsigned mask);

extern int host_range(uint32_t ip, uint32_t bits_mask, uint32_t *head, uint32_t *tail);

/* "192.168.1.1" 转换为主机字节序的网络地址 */
extern uint32_t ip_str_to_value(const char *ip);


#endif /* COMMON_H_20240820181327_0X0CADDBF7 */
