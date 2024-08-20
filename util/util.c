#include <stdint.h>
#include "common.h"


unsigned host_count(unsigned mask)
{
	int bits = 32 - mask;
	return (1<<bits) - 2;
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

/* "192.168.1.1" 转换为主机字节序的网络地址 */
uint32_t ip_str_to_value(const char *ip)
{
	if (ip == NULL) {
		return 0;
	}

    uint32_t ipv4_addr;
    inet_pton(AF_INET, ip, (void *) &ipv4_addr);

	return ntohl(ipv4_addr);
}
