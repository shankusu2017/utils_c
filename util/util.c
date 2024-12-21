#include <stdint.h>
#include "common.h"


unsigned host_count(unsigned mask)
{
	int bits = 32 - mask;
	return (1<<bits) - 2;   /* 第一个用作 gateway, 最后一个用于掩码 */
}
/*
 * 给出 192.168.15.1/24 的情况下 能分配的主机地址范围
 * 按照规则，192.168.15.0 192.168.15.255 这两个地址不能分配
 */
int host_range(uint32_t ip, uint32_t bits_mask, uint32_t *head, uint32_t *tail)
{
	uint32_t min = 0;
	uint32_t max = 0;

	/* 网络号 掩码 */
	uint32_t mask = 0xffffffff;
	uint32_t net_mask = (mask<<(32-bits_mask));	/* 0xffffff00 */
	uint32_t host_mask = (mask >> bits_mask);	/* 0x000000ff */

	*head = (ip & net_mask) + 1;	/* 192.168.15.1 */
	*tail = (ip & net_mask) + host_mask - 1;	/* 192.168.15.254 */
}

extern int
mac_hexstr_to_digit(const char *mac_hex_str, uint8_t mac_digit[MAC_LEN_BYTES])
{
    int i = 0;
    uint8_t val = 0;
    const char *pos = NULL;
    for (i = 0, pos = mac_hex_str; i < MAC_LEN_BYTES && *pos; pos++) {
        if ('A' <= *pos && 'F' >= *pos) {
            val = *pos - 'A' + 10;
        } else if ('a' <= *pos && 'f' >= *pos) {
            val = *pos - 'a' + 10;
        } else if ('0' <= *pos && '9' >= *pos) {
            val = *pos - '0';
        } else {
            continue;
        }

        val <<= 4;
        pos++;
        if ('A' <= *pos && 'F' >= *pos) {
            val |= *pos - 'A' + 10;
        } else if ('a' <= *pos && 'f' >= *pos) {
            val |= *pos - 'a' + 10;
        } else if ('0' <= *pos && '9' >= *pos) {
            val |= *pos - '0';
        } else {
            return -1;
        }

        mac_digit[i] = val;
        i++;
    }

    if (i != MAC_LEN_BYTES) {
        return -1;
    }

    return 0;
}

extern int
mac_digit_to_hexstr(char mac_hex_str[MAC_LEN_STR_00], const uint8_t mac_digit[MAC_LEN_BYTES])
{
    int len = sprintf(mac_hex_str, "%02x:%02x:%02x:%02x:%02x:%02x", 
 		              mac_digit[0], mac_digit[1], mac_digit[2],
 		              mac_digit[3], mac_digit[4], mac_digit[5]);

    return len == (MAC_LEN_STR_00 - 1) ? 0 : -1;
}

extern int
mac_hexstr_to_lower(const char *mac_hex_str, char lower[MAC_LEN_STR_00])
{
	uint8_t mac_digit[MAC_LEN_BYTES]= {0};
	lower[0] = 0;

	int ret = mac_hexstr_to_digit(mac_hex_str, mac_digit);
	if (ret) {
		return ret;
	}

	ret = mac_digit_to_hexstr(lower, mac_digit);
	return ret;
}

extern void
mac_to_lower(char mac_hex_str[MAC_LEN_STR_00])
{
	mac_hex_str[MAC_LEN_STR_00 - 1] = 0;

	for (int i = 0; i < 18 - 1; ++i) {
		if (mac_hex_str[i] >= 'A' && mac_hex_str[i] <= 'Z') {
			mac_hex_str[i] += ('a' - 'A');
		}
	}
}

/* 主机序的 ip 值转为 "192.168.123.251" */
extern void
ipv4_host_to_str(uint32_t ip, char *ip_str, size_t len)
{
	ip = htonl(ip);
	inet_ntop(AF_INET, &ip, ip_str, len);
}


/* "192.168.1.1" 转换为主机字节序的网络地址 */
extern uint32_t
ipv4_str_to_host(const char *ip)
{
	if (ip == NULL) {
		return 0;
	}

    uint32_t ipv4_addr;
    inet_pton(AF_INET, ip, (void *) &ipv4_addr);

	return ntohl(ipv4_addr);
}
