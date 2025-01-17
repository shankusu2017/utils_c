#ifndef RANDOM_H_20240820231000_0X256E0BB4
#define RANDOM_H_20240820231000_0X256E0BB4

#include "uc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 超高的随机性，可能阻塞，直到系统产生足够的墒 */
int uc_random(void *buf, size_t len);

/* 比上面的稍弱，不阻塞 */
int uc_urandom(void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* RANDOM_H_20240820231000_0X256E0BB4 */
