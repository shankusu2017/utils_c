#include <stdio.h>
#include <stdint.h>
#include "list.h"
#include "common.h"

typedef struct packet_s {
    int zh;
	int val;
    int zt;
    struct list_head  list;	/* 必须放最后面 */
} packet_t;

int
main(int argc, char *argv[])
{
    struct list_head list_head;
    INIT_LIST_HEAD(&list_head);

    int i = 1;
    packet_t pkt1, pkt2, pkt3, pkt4;
    pkt1.val = i++;
    pkt2.val = i++;
    pkt3.val = i++;
    pkt4.val = i++;
    pkt1.zh = pkt1.zt = pkt2.zh = pkt2.zt = pkt3.zh = pkt3.zt = pkt4.zh = pkt4.zt = 0;

    list_add_tail(&pkt1.list, &list_head);
    list_add_tail(&pkt2.list, &list_head);
    list_add_tail(&pkt3.list, &list_head);
    list_add_tail(&pkt4.list, &list_head);

    struct list_head *node;

    packet_t *pkt;
    list_for_each (node, &list_head) {
        pkt = list_entry(node, packet_t, list);
        printf(".pkt->val: %d\n", pkt->val);
    }
    for (int i = 1; i < 32; i++) {
        printf("mask: %d, count: %u\n", i, uc_utils_host_count(i));
    }

    uint32_t head, tail;
    uc_utils_host_range(0xc0a80f01, 12, &head, &tail);
    printf("head: %x, tail: %x\n", head, tail);

    int tmp = pkt1.zh++;
    printf("tmp: %d, hi: %d\n", tmp, pkt1.zh);
}
