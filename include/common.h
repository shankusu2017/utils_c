#ifndef COMMON_H_20240820181327_0X0CADDBF7
#define COMMON_H_20240820181327_0X0CADDBF7

#ifdef __cplusplus
extern "C" {
#endif

/* 内部用 */
#ifndef _WIN32
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#include <linux/unistd.h>
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
#include <sys/stat.h>
#include <sys/types.h>          /* See NOTES */
#include <signal.h>
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>


#ifndef MAC_LEN_BYTES
#define MAC_LEN_BYTES (6)
#endif

#ifndef MAC_LEN_STR_00
#define MAC_LEN_STR_00 (18)
#endif


#ifndef IPV4_LEN_STR_00
#define IPV4_LEN_STR_00 (16)
#endif

#ifndef ID_CHAR_32_STR_00
#define ID_CHAR_32_STR_00  (33)
#endif

#ifndef ID_CHAR_64_STR_00
/* 86c14d9ebf144b84b06abf0f73cc82a686c14d9ebf144b84b06abf0f73cc82a6 */
#define ID_CHAR_64_STR_00  (65)
#endif

#ifndef HASH_NAME_LEN_00
#define HASH_NAME_LEN_00 (257)
#endif


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

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H_20240820181327_0X0CADDBF7 */
