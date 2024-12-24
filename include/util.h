#ifndef UTIL_H_20240620111556_0X789B27EC
#define UTIL_H_20240620111556_0X789B27EC

/* util */

#include "common.h"

extern int uc_utils_mac_hexstr_to_digit(const char *mac_hex_str, uint8_t mac_digit[MAC_LEN_BYTES]);

extern int uc_utils_mac_digit_to_hexstr(char mac_hex_str[MAC_LEN_STR_00], const uint8_t mac_digit[MAC_LEN_BYTES]);

extern int uc_utils_mac_hexstr_to_lower(const char *mac_hex_str, char lower[MAC_LEN_STR_00]);

extern void uc_utils_mac_to_lower(char mac_hex_str[MAC_LEN_STR_00]);

/* 主机序的 ip 值转为 "192.168.123.251" */
extern void uc_utils_ipv4_host_to_str(uint32_t ip, char *ip_str, size_t len);

/* "192.168.1.1" 转换为主机字节序的网络地址 */
extern uint32_t uc_utils_ipv4_str_to_host(const char *ip);

extern unsigned uc_utils_host_count(unsigned mask);

extern int uc_utils_host_range(uint32_t ip, uint32_t bits_mask, uint32_t *head, uint32_t *tail);

#endif /* UTIL_H_20240620111556_0X789B27EC */
