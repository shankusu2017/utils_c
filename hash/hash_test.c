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

int test_table(void)
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
    hash_table_t *tbl = hash_create(1024*64, sizeof(int));
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
        item = hash_next(tbl, ((hash_node_t *)item)->key);
    }
    assert(ttl == 3);

    /* 测试大规模插入 */
    printf("------> insert start\n");
    int64_t nA = utils_printfms();
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
    assert(hash_node_ttl(ttl) == big_insert_ttl + 3);
    int64_t dA = utils_printfms();
    printf("<----- insert done, node(%d) cost: %ldms\n", big_insert_ttl, dA - nA);

    /* 测试循环性能 */
    nA = utils_printfms();
    ttl = 0;
    hash_node_t *item = hash_next(tbl, NULL);
    hash_node_t *next = NULL;
    printf("------>loop start\n");
    while (item) {
        ++ttl;
        //printf("tbl.ttl: %d, val: %d\n", hash_node_ttl(tbl), *(int *)(item->val));
        next = hash_next(tbl, item->key);
        assert(*(int *)item->key == *(int *)item->val);
		//hash_delete(tbl, item->key);
        item = next;
    }
    assert(ttl == hash_node_ttl(tbl));
    printf("<------ loop done, ttl: %d, hash.item.ttl: %d\n", ttl, hash_node_ttl(tbl));
    printf("\nhash next and delete done, cost: %ldms\n", utils_printfms() - nA);


    /* 测试删除逻辑 */
    printf("delete start, node.ttl: %d,  table.ttl: %d\n", ttl, hash_node_ttl(tbl));
    item = hash_next(tbl, NULL);
    next = NULL;
    nA = utils_printfms();
    ttl = 0;
    int tbl_ttl = hash_node_ttl(tbl);
    while (item) {
        next = hash_next(tbl, item->key);
        hash_delete(tbl, item->key);
        item = next;
        ttl++;
    }
    printf("delete done, node.ttl: %d,  table.ttl: %d, cost: %ldms\n", ttl, hash_node_ttl(tbl), utils_printfms() - nA);
    assert(tbl_ttl == ttl);
    assert(hash_node_ttl(ttl) == 0);

    printf("<======== hash table test done\n");
}

int test_new_table(void)
{
    int ttl = 1024*1024;
    int tbl_height = ttl / 4;
    t_node_t *node = (t_node_t *)calloc(ttl, sizeof(t_node_t));
    assert(NULL != node);

    for (int i = 0; i < ttl; ++i) {
        node[i].k = i;
        int *v = (int *)malloc(sizeof(int));
        assert(v != NULL);

        *v = t_cal_key(&i, sizeof(i));
        node[i].v = v;
    }


    hash_table_t *new_tbl = hash_short_create(tbl_height, hash_short_key_int32);

    /* 测试插入逻辑 */
    printf("insert start, node.ttl: %d,  table.ttl: %d\n", ttl, hash_node_ttl(new_tbl));
    int64_t msA = utils_printfms();
    hash_short_key_t new_key;
    for (int i = 0; i < ttl; ++i) {
        new_key.i32 = node[i].k;
        hash_short_insert(new_tbl, new_key, node[i].v);
    }
    assert(hash_node_ttl(new_tbl) == ttl);
    printf("insert done, node.ttl: %d,  table.ttl: %d, cost: %ldms\n", ttl, hash_node_ttl(new_tbl), utils_printfms() - msA);

    /* 测试循环、查找、逻辑 */
    printf("loop start, node.ttl: %d,  table.ttl: %d\n", ttl, hash_node_ttl(new_tbl));
    hash_node_t *item = hash_short_head(new_tbl);
    hash_node_t *next = NULL;
    msA = utils_printfms();
    int loop_ttl = 0;
    while (item) {
        next = hash_short_next(new_tbl, item->short_key);
        int v = t_cal_key(&item->short_key.i32, sizeof(item->short_key.i32));
        int v2 = *((int *)item->val);
        assert(v == v2);
        //hash_short_delete(new_tbl, item->short_key);
        item = next;
        loop_ttl++;
    }
    printf("loop done, node.ttl: %d,  table.ttl: %d, cost: %ldms\n", loop_ttl, hash_node_ttl(new_tbl), utils_printfms() - msA);
    assert(loop_ttl == ttl);
    assert(hash_node_ttl(new_tbl) == ttl);

    /* 测试删除逻辑 */
    printf("delete start, node.ttl: %d,  table.ttl: %d\n", ttl, hash_node_ttl(new_tbl));
    item = hash_short_head(new_tbl);
    next = NULL;
    msA = utils_printfms();
    loop_ttl = 0;
    while (item) {
        next = hash_short_next(new_tbl, item->short_key);
        hash_short_delete(new_tbl, item->short_key);
        item = next;
        loop_ttl++;
    }
    printf("delete done, node.ttl: %d,  table.ttl: %d, cost: %ldms\n", loop_ttl, hash_node_ttl(new_tbl), utils_printfms() - msA);
    assert(loop_ttl == ttl);
    assert(hash_node_ttl(new_tbl) == 0);

    printf("<======== hash new table test done\n");
}

int main(int argc, char *argv[])
{
    return (test_table() || test_new_table());
}
