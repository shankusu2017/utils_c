#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "hash.h"
#include "memory.h"
#include "common.h"


int test_table_void(void)
{
    hash_table_t *tbl = hash_create(1024*64, hash_key_void, sizeof(int));
    const size_t large_size = 1024*2048;

    {
        /* same key */
        int key = 1;
        int *val = NULL;
        for (int i = 0; i < large_size; ++i) {
            val = (int *)util_malloc(sizeof(int));
            if (NULL == val) {
                printf("oom\n");
                exit(-1);
            } else {
                *val = i;
                hash_insert(tbl, &key, val);
                assert(hash_node_ttl(tbl) == 1);
            }
        }
        assert(hash_node_ttl(tbl) == 1);
        hash_delete(tbl,  &key);
        assert(hash_node_ttl(tbl) == 0);
    }


    {   /* unique key */
        int key = 0;
        int *val = NULL;
        for (int i = 0; i < large_size; ++i) {
            val = (int *)util_malloc(sizeof(int));
            if（NULL == val）{
                printf("oom\n");
                exit(-1);
            } else {
                key = *val = i;
                hash_insert(tbl, &key, val);
                assert(hash_node_ttl(tbl) == i+1);
            }
        }
    }

     {   /* look */
        int *val = NULL;
        for (int i = 0; i < large_size; ++i) {
            hash_node_t *node = hash_find(tbl, i);
            assert(node != NULL);
            assert(i == *((int *)node->val));
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
    printf("loop done, node.ttl: %d,  cost: %ldms\n", large_size, utils_ms() - nA);


    /* 测试 delete */
    nA = utils_ms();
    ttl = hash_node_ttl(tbl);
    hash_node_t *item = hash_next(tbl, NULL);
    hash_node_t *next = NULL;
    while (item) {
        next = hash_next(tbl, hash_key_addr(tbl, item));
        hash_delete(tbl, hash_key_addr(tbl, item));
        item = next;
        assert(hash_node_ttl(tbl) == --ttl);
    }
    assert(hash_node_ttl(tbl) == 0);
    printf("delete done, node.ttl: %d,  cost: %ldms\n", large_size, utils_ms() - nA);

    /* test memory */
    hash_free(tbl);
    assert(0== util_malloc_used_memory());

    printf("<======== hash table test done\n");
}

int test_table_keys(void)
{
    printf("================> table.keys test start\n");

    size_t node_ttl = 1024*4096;
    hash_table_t *tbl = hash_create(node_ttl, hash_key_uint32, 0);
    assert(tbl != NULL);

    // test insert
    for (size_t i = 0; i < node_ttl; ++i) {
        hash_key_t key;
        key.u32 = i;
        if (i % 4) {
            int ret = hash_insert(tbl, &key.u32, NULL);
            assert(ret == 0);
        }
    }
    // next and delete
    size_t ttl = hash_node_ttl(tbl);
    hash_node_t *item = hash_next(tbl, NULL);
    hash_node_t *next = NULL;
    while (item) {
        next = hash_next(tbl, hash_key_addr(tbl, item));
        hash_delete(tbl, hash_key_addr(tbl, item));
        item = next;
        assert(hash_node_ttl(tbl) == --ttl);
    }
    assert(hash_node_ttl(tbl) == 0);

    /* test memory */
    hash_free(tbl);
    assert(0== util_malloc_used_memory());

    printf("<================ table.keys test done\n");
    return 0;
}


int main(int argc, char *argv[])
{
    return (test_table_void() || test_table_keys());
}
