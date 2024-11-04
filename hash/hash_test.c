#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "hash.h"
#include "memory.h"
#include "common.h"


int test_table_void(void)
{
    printf("0x5b1ccf5a test void hash\n");

    const size_t large_size = 1024*32;
    hash_table_t *tbl = hash_create(large_size*2, hash_key_mem, sizeof(int));

    printf("mem.size:%u\n", util_malloc_used_memory());

    {
        /* same key, diff val */
        hash_key_t key;
        int key_val = 1;
        key.mem = &key_val;
        int *val = NULL;
        for (int i = 0; i < large_size; ++i) {
            val = (int *)util_malloc(sizeof(int));
            if (NULL == val) {
                printf("oom\n");
                exit(-1);
            } else {
                *val = i;
                hash_insert(tbl, key, val);
                assert(hash_node_ttl(tbl) == 1);
            }
        }
        printf("mem.size:%u\n", util_malloc_used_memory());
        assert(hash_node_ttl(tbl) == 1);
        hash_delete(tbl, key);
        assert(hash_node_ttl(tbl) == 0);
    }


    {   /* unique key */
        hash_key_t key;
        int *val = NULL, key_val = 0;
        for (int i = 0; i < large_size; ++i) {
            val = (int *)util_malloc(sizeof(int));
            key_val = i;
            if (NULL == val) {
                printf("oom\n");
                exit(-1);
            } else {
                *val = i;
                key.mem = &key_val;
                hash_insert(tbl, key, val);
                assert(hash_node_ttl(tbl) == i+1);
            }
        }
        printf("mem.size:%u\n", util_malloc_used_memory());
    }

    {   /* look */
        int key_val = 0;
        hash_key_t key;
        key.mem = &key_val;
        for (int i = 0; i < large_size; ++i) {
            key_val = i;
            hash_node_t *node = hash_find(tbl, key);
            assert(node != NULL);
            assert(i == *((int *)hash_node_val(node)));
        }
    }

    /* 测试 loop  */
    int64_t nA = utils_ms();
    size_t ttl = hash_node_ttl(tbl);
    hash_node_t *item = hash_next(tbl, NULL);
    hash_node_t *next = NULL;
    while (item) {
        next = hash_next(tbl, hash_key_addr(tbl, item));
        item = next;
        ttl--;
    }
    assert(ttl == 0);
    printf("0x70b354d1 loop done, node.ttl: %ld,  cost: %ldms\n", large_size, utils_ms() - nA);


    /* 测试 delete */
    {
        nA = utils_ms();
        ttl = hash_node_ttl(tbl);
        hash_node_t *item = hash_next(tbl, NULL);
        hash_node_t *next = NULL;
        while (item) {
            next = hash_next(tbl, hash_key_addr(tbl, item));
            hash_delete(tbl, *(hash_key_t *)hash_key_addr(tbl, item));
            item = next;
            assert(hash_node_ttl(tbl) == --ttl);
        }
        assert(hash_node_ttl(tbl) == 0);
        printf("delete done, node.ttl: %ld,  cost: %ldms\n", large_size, utils_ms() - nA);
    }

    /* test memory */
    hash_free(tbl);
    assert(0== util_malloc_used_memory());

    printf("0x3fe98825 <======== void hash table test done\n");
    return 0;
}

int test_table_keys(void)
{
    printf("0x7efbb6ff ================> table.keys test start\n");

    size_t node_ttl = 1024*16;
    hash_table_t *tbl = hash_create(node_ttl/2, hash_key_uint32, 0);
    assert(tbl != NULL);

    // test insert
    for (size_t i = 0; i < node_ttl; ++i) {
        hash_key_t key;
        key.u32 = i;
        if (i % 4) {
            int ret = hash_insert(tbl, key, NULL);
            assert(ret == 0);
        }
    }
    // next and delete
    size_t ttl = hash_node_ttl(tbl);
    hash_node_t *item = hash_next(tbl, NULL);
    hash_node_t *next = NULL;
    while (item) {
        next = hash_next(tbl, hash_key_addr(tbl, item));
        hash_delete(tbl, *(hash_key_t *)hash_key_addr(tbl, item));
        item = next;
        assert(hash_node_ttl(tbl) == --ttl);
    }
    assert(hash_node_ttl(tbl) == 0);

    {   /* unique key 
         * insert AND find 
         */
        hash_key_t key;
        int *val = NULL;
        for (int i = 0; i < node_ttl; ++i) {
            val = (int *)util_malloc(sizeof(int));
            if (NULL == val) {
                printf("oom\n");
                exit(-1);
            } else {
                key.i32 = *val = i;
                hash_insert(tbl, key, val);
                assert(hash_node_ttl(tbl) == i+1);
            }
        }
        for (int i = 0; i < node_ttl; ++i) {
            key.i32 = i;
            hash_node_t *node = hash_find(tbl, key);
            assert(node != NULL);
            assert(i == *((int *)hash_node_val(node)));
        }
    }


    /* test memory */
    hash_free(tbl);
    assert(0== util_malloc_used_memory());

    printf("0x0e6b0583 <================ table.keys test done\n");
    return 0;
}


int main(int argc, char *argv[])
{
    int ret = (test_table_void() || test_table_keys());
    assert(ret == 0);
    return 0;
}
