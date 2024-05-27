#ifndef UTIL_H_20240620111556_0X789B27EC
#define UTIL_H_20240620111556_0X789B27EC

#include <stdint.h>

extern unsigned host_count(unsigned mask);

extern int host_range(uint32_t ip, uint32_t bits_mask, uint32_t *head, uint32_t *tail);

#endif /* UTIL_H_20240620111556_0X789B27EC */
