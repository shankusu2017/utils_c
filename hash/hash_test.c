#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "hash.h"

typedef struct t_node_s {
    int k;
    void *v;
} t_node_t;

static size_t t_cal_key(void *buf, size_t len)
{
    size_t seed = 0x460613c7 + len;     /* seed */
    size_t step = (len >> 5) + 1;       /* 预防 len 过大 */
	char *data = (char *)buf;

    for (; len >= step; len -= step) {  /* compute hash */
        seed = seed ^ ((seed << 5) + (seed >> 2) + (unsigned char)(data[len - 1]));
    }

    return seed;
}

int test_table_void(void)
{
    int key1 = -1;

    int *val1 = (int *)malloc(sizeof(int));
    *val1 = 1;

    int *val11 = (int *)malloc(sizeof(int));
    *val11 = 11;

    int key2 = -2;
    int *val2 = (int *)malloc(sizeof(int));
    *val2 = 200;

    int key3 = -3;
    int *val3 = (int *)malloc(sizeof(int));
    *val3 = 300;

    /* 测试 insert 中出现 key 相同的情况 */
    hash_table_t *tbl = hash_create(1024*64, hash_key_void, sizeof(int));
    hash_insert(tbl, &key1, val1);
    hash_insert(tbl, &key1, val11);
    assert(hash_node_ttl(tbl) == 1);

    hash_insert(tbl, &key2, val2);
    assert(hash_node_ttl(tbl) == 2);
    hash_insert(tbl, &key3, val3);
    assert(hash_node_ttl(tbl) == 3);

    /* 测试 loop 次数 */
    int ttl = 0;
    hash_node_t *item = hash_next(tbl, NULL);
    while (item) {
        ++ttl;
        //printf("val: %d\n", *(int *)(((hash_node_t *)item)->val));
        item = hash_next(tbl,  hash_key_addr(tbl, item));
    }
    assert(ttl == 3);

    /* 删除零散的不规则数据 */
    item = hash_next(tbl, NULL);
    hash_node_t *next = NULL;
    int64_t nA = utils_ms();
    int tbl_ttl = hash_node_ttl(tbl);
    while (item) {
        next = hash_next(tbl, hash_key_addr(tbl, item));
        hash_delete(tbl, hash_key_addr(tbl, item));
        item = next;
    }
    assert(hash_node_ttl(tbl) == 0);

    /* 测试大规模插入 */
    printf("------> insert start\n");
    nA = utils_ms();
    int big_insert_ttl = 1024*64;
    for (int i = 0; i < big_insert_ttl; ++i) {
        int *key_i = (int*)malloc(sizeof(int));
        assert(key_i);
        *key_i = i;
        int *val_i = (int *)malloc(sizeof(int));
        assert(val_i);
        *val_i = i;
        hash_insert(tbl, key_i, val_i);
    }
    assert(hash_node_ttl(tbl) == big_insert_ttl);
    int64_t dA = utils_ms();
    printf("<----- insert done, node(%d) cost: %ldms\n", big_insert_ttl, dA - nA);

    /* 测试循环性能 */
    nA = utils_ms();
    ttl = 0;
    item = hash_next(tbl, NULL);
    next = NULL;
    printf("------>loop start\n");
    while (item) {
        ++ttl;
        //printf("tbl.ttl: %d, val: %d\n", hash_node_ttl(tbl), *(int *)(item->val));
        next = hash_next(tbl,  hash_key_addr(tbl, item));
        assert(*(int *)hash_key_addr(tbl, item) == *(int *)item->val);
		//hash_delete(tbl, hash_key_addr(tbl, item));
        item = next;
    }
    assert(ttl == hash_node_ttl(tbl));
    printf("<------ loop done, ttl: %d, hash.item.ttl: %d\n", ttl, hash_node_ttl(tbl));
    printf("\nhash next and delete done, cost: %ldms\n", utils_ms() - nA);


    /* 测试删除逻辑 */
    printf("delete start, node.ttl: %d,  table.ttl: %d\n", ttl, hash_node_ttl(tbl));
    item = hash_next(tbl, NULL);
    next = NULL;
    nA = utils_ms();
    ttl = 0;
    tbl_ttl = hash_node_ttl(tbl);
    while (item) {
        next = hash_next(tbl, hash_key_addr(tbl, item));
        hash_delete(tbl, hash_key_addr(tbl, item));
        item = next;
        ttl++;
    }
    printf("delete done, node.ttl: %d,  table.ttl: %d, cost: %ldms\n", ttl, hash_node_ttl(tbl), utils_ms() - nA);
    assert(tbl_ttl == ttl);
    assert(hash_node_ttl(tbl) == 0);

    printf("<======== hash table test done\n");
}

int test_table_keys()
{
    printf("================> table.keys test start\n");

    size_t node_ttl = 1024*1024;
    hash_table_t *tbl = hash_create(node_ttl, hash_key_int32_unsigned, 0);
    assert(tbl != NULL);

    // test insert
    for (size_t i = 0; i < node_ttl; ++i) {
        hash_key_t key;
        key.u32 = i;
        int ret = hash_insert(tbl, &key.u32, NULL);
        assert(ret == 0);
    }

    // test find
    for (size_t i = 0; i < node_ttl; ++i) {
        hash_key_t key;
        key.u32 = i;
        hash_node_t *node = hash_find(tbl, &key.u32);
        assert(NULL != node);
    }
    for (size_t i = node_ttl; i < node_ttl*2; ++i) {
        hash_key_t key;
        key.u32 = i;
        hash_node_t *node = hash_find(tbl, &key.u32);
        assert(NULL == node);
    }

    //  test delete
    hash_node_t * item = hash_next(tbl, NULL);
    hash_node_t *next = NULL;
    size_t tbl_ttl = hash_node_ttl(tbl);
    size_t ttl = 0;
    while (item) {
        next = hash_next(tbl, hash_key_addr(tbl, item));
        hash_delete(tbl, hash_key_addr(tbl, item));
        item = next;
        ttl++;
    }
    assert(ttl = tbl_ttl);
    assert(0 == hash_node_ttl(tbl));

    // test find 2
    for (size_t i = 0; i < node_ttl; ++i) {
        hash_key_t key;
        key.u32 = i;
        hash_node_t *node = hash_find(tbl, &key.u32);
        assert(NULL == node);
    }
    printf("<================ table.keys test done\n");
    return 0;
}


int main(int argc, char *argv[])
{
    return (test_table_void() || test_table_keys());
}
