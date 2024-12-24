#include "hash.h"
#include "memory.h"
#include "common.h"


int test_table_void(void)
{
    printf("0x5b1ccf5a test void hash\n");

    const size_t large_size = 1024*64;
    uc_hash_table_t *tbl = uc_hash_create(large_size*2, uc_hash_key_mem, sizeof(int));

    printf("mem.size:%lu\n", uc_hash_test_mem_used());

    {
        /* same key, diff val */
        uc_hash_key_t key;
        int key_val = 1;
        key.mem = &key_val;
        int *val = NULL;
        for (int i = 0; i < large_size; ++i) {
            val = (int *)uc_hash_test_mem_malloc(sizeof(int));
            if (NULL == val) {
                printf("oom\n");
                exit(-1);
            } else {
                *val = i;
                uc_hash_insert(tbl, key, val);
                assert(uc_hash_node_ttl(tbl) == 1);
            }
        }
        printf("mem.size:%lu\n", uc_hash_test_mem_used());
        assert(uc_hash_node_ttl(tbl) == 1);
        uc_hash_delete(tbl, key);
        assert(uc_hash_node_ttl(tbl) == 0);
    }


    {   /* unique key */
        uc_hash_key_t key;
        int *val = NULL, key_val = 0;
        for (int i = 0; i < large_size; ++i) {
            val = (int *)uc_hash_test_mem_malloc(sizeof(int));
            key_val = i;
            if (NULL == val) {
                printf("oom\n");
                exit(-1);
            } else {
                *val = i;
                key.mem = &key_val;
                uc_hash_insert(tbl, key, val);
                assert(uc_hash_node_ttl(tbl) == i+1);
            }
        }
        printf("mem.size:%lu\n", uc_hash_test_mem_used());
    }

    {   /* look */
        int key_val = 0;
        uc_hash_key_t key;
        key.mem = &key_val;
        for (int i = 0; i < large_size; ++i) {
            key_val = i;
            uc_hash_node_t *node = uc_hash_find(tbl, key);
            assert(node != NULL);
            assert(i == *((int *)uc_hash_node_val(node)));
        }
    }

    pthread_rwlock_t f_rdlock_usr_id;
    pthread_rwlock_init(&f_rdlock_usr_id, NULL);
    /* 测试 loop  */
    int64_t nA = uc_time_ms();
    size_t ttl = uc_hash_node_ttl(tbl);
    uc_hash_node_t *item = uc_hash_next(tbl, NULL);
    uc_hash_node_t *next = NULL;
    while (item) {
        pthread_rwlock_rdlock(&f_rdlock_usr_id);
        next = uc_hash_next(tbl, uc_hash_key_addr(item));
        pthread_rwlock_unlock(&f_rdlock_usr_id);
        item = next;
        ttl--;
    }
    assert(ttl == 0);
    printf("0x70b354d1 with rwlock loop done, node.ttl: %ld,  cost: %ldms\n", large_size, uc_time_ms() - nA);


    /* 测试 delete */
    {
        nA = uc_time_ms();
        ttl = uc_hash_node_ttl(tbl);
        uc_hash_node_t *item = uc_hash_next(tbl, NULL);
        uc_hash_node_t *next = NULL;
        while (item) {
            next = uc_hash_next(tbl, uc_hash_key_addr(item));
            uc_hash_delete(tbl, *(uc_hash_key_t *)uc_hash_key_addr(item));
            item = next;
            assert(uc_hash_node_ttl(tbl) == --ttl);
        }
        assert(uc_hash_node_ttl(tbl) == 0);
        printf("delete done, node.ttl: %ld,  cost: %ldms\n", large_size, uc_time_ms() - nA);
    }

    /* test memory */
    uc_hash_free(tbl);
    assert(0== uc_hash_test_mem_used());

    printf("0x3fe98825 <======== void hash table test done\n");
    return 0;
}

static
int test_table_keys(void)
{
    printf("0x7efbb6ff ================> table.keys test start\n");

    size_t node_ttl = 1024*16;
    uc_hash_table_t *tbl = uc_hash_create(node_ttl/2, uc_hash_key_uint32, 0);
    assert(tbl != NULL);

    // test insert
    for (size_t i = 0; i < node_ttl; ++i) {
        uc_hash_key_t key;
        key.u32 = i;
        if (i % 4) {
            int ret = uc_hash_insert(tbl, key, NULL);
            assert(ret == 0);
        }
    }
    // next and delete
    size_t ttl = uc_hash_node_ttl(tbl);
    uc_hash_node_t *item = uc_hash_next(tbl, NULL);
    uc_hash_node_t *next = NULL;
    while (item) {
        next = uc_hash_next(tbl, uc_hash_key_addr(item));
        uc_hash_delete(tbl, *(uc_hash_key_t *)uc_hash_key_addr(item));
        item = next;
        assert(uc_hash_node_ttl(tbl) == --ttl);
    }
    assert(uc_hash_node_ttl(tbl) == 0);

    {   /* unique key
         * insert AND find
         */
        uc_hash_key_t key;
        int *val = NULL;
        for (int i = 0; i < node_ttl; ++i) {
            val = (int *)uc_hash_test_mem_malloc(sizeof(int));
            if (NULL == val) {
                printf("oom\n");
                exit(-1);
            } else {
                key.i32 = *val = i;
                uc_hash_insert(tbl, key, val);
                assert(uc_hash_node_ttl(tbl) == i+1);
            }
        }
        for (int i = 0; i < node_ttl; ++i) {
            key.i32 = i;
            uc_hash_node_t *node = uc_hash_find(tbl, key);
            assert(node != NULL);
            assert(i == *((int *)uc_hash_node_val(node)));
        }
    }


    /* test memory */
    uc_hash_free(tbl);
    assert(0== uc_hash_test_mem_used());

    printf("0x0e6b0583 <================ table.keys test done\n");
    return 0;
}

static
int test_table_key_int16(void)
{
    printf("0x7efbb6ff <================ table.key.int16 test start\n");

    size_t larget_ttl = (1<<16)*8;
    uc_hash_table_t *tbl = uc_hash_create(larget_ttl*2, uc_hash_key_int16, 0);
    assert(tbl != NULL);

    size_t node_ttl = (1<<16);  /* key 溢出，正好测试 key 覆盖的情况 */
    // test insert and find
    {   int ttl = 0;
        for (size_t i = 0; i < larget_ttl; ++i) {
            uc_hash_key_t key;
            key.i16 = i;
            ttl++;
            int ret = uc_hash_insert(tbl, key, NULL);
            assert(ret == 0);
        }
        assert(node_ttl == uc_hash_node_ttl(tbl));
        for (size_t i = 0; i < larget_ttl; ++i) {
            uc_hash_key_t key;
            key.i16 = i;
            uc_hash_node_t *node = uc_hash_find(tbl, key);
            assert(NULL != node);
            void *val = uc_hash_node_val(node);
            assert(NULL == val);
        }
    }

    {
        // next
        size_t ttl = uc_hash_node_ttl(tbl);
        assert(node_ttl == ttl);
        uc_hash_node_t *item = uc_hash_next(tbl, NULL);
        uc_hash_node_t *next = NULL;
        size_t times = 0;
        while (item) {
            next = uc_hash_next(tbl, uc_hash_key_addr(item));
            // uc_hash_delete(tbl, *(hash_key_t *)uc_hash_key_addr(item));
            item = next;
            times++;
            // assert(uc_hash_node_ttl(tbl) == --ttl);
        }
        assert(times == ttl);
    }

    {
        // delete
        size_t ttl = uc_hash_node_ttl(tbl);
        assert(node_ttl == ttl);
        uc_hash_node_t *item = uc_hash_next(tbl, NULL);
        uc_hash_node_t *next = NULL;
        size_t times = 0;
        while (item) {
            next = uc_hash_next(tbl, uc_hash_key_addr(item));
            uc_hash_delete(tbl, *(uc_hash_key_t *)uc_hash_key_addr(item));
            item = next;
            times++;
            // assert(uc_hash_node_ttl(tbl) == --ttl);
        }
        assert(times == ttl);
        assert(uc_hash_node_ttl(tbl) == 0);

        for (size_t i = 0; i < larget_ttl; ++i) {
            uc_hash_key_t key;
            key.i16 = i;
            uc_hash_node_t *node = uc_hash_find(tbl, key);
            assert(NULL == node);
        }
    }

    {   /* unique key and val
         * insert AND find
         */
        assert(uc_hash_node_ttl(tbl) == 0);
        uc_hash_key_t key;
        int *val = NULL;
        for (int i = 0; i < (1<<16); ++i) {
            val = (int *)uc_hash_test_mem_malloc(sizeof(int));
            if (NULL == val) {
                printf("oom\n");
                exit(-1);
            } else {
                key.i16 = *val = i;
                uc_hash_insert(tbl, key, val);
                if ((i+1) < (1<<16)) {
                    assert(uc_hash_node_ttl(tbl) == i + 1);
                } else {
                    assert(uc_hash_node_ttl(tbl) == 1 << 16);
                }
            }
        }
        for (int i = 0; i < (1<<16); ++i) {
            key.i16 = i;
            uc_hash_node_t *node = uc_hash_find(tbl, key);
            assert(node != NULL);
            assert(i == *((int *)uc_hash_node_val(node)));
        }
    }


    /* test memory */
    uc_hash_free(tbl);
    assert(0== uc_hash_test_mem_used());

    printf("0x0e6b0583 ================> table.key.int64 test done\n");
    return 0;
}


static
int test_table_key_int64(void)
{
    printf("0x7efbb6ff <================ table.key.int64 test start\n");

    size_t larget_ttl = 1024*64;
    size_t node_ttl = 0;
    uc_hash_table_t *tbl = uc_hash_create(larget_ttl*2, uc_hash_key_int64, 0);
    assert(tbl != NULL);

    // test insert and find
    {   int ttl = 0;
        for (size_t i = 0; i < larget_ttl; ++i) {
            uc_hash_key_t key;
            key.i64 = i;
            if (i % 4) {
                ttl++;
                node_ttl++;
                int ret = uc_hash_insert(tbl, key, NULL);
                assert(ret == 0);
            }
        }
        assert(ttl == uc_hash_node_ttl(tbl));
        for (size_t i = 0; i < larget_ttl; ++i) {
            uc_hash_key_t key;
            key.i64 = i;
            if (i % 4) {
                uc_hash_node_t *node = uc_hash_find(tbl, key);
                assert(NULL != node);
                void *val = uc_hash_node_val(node);
                assert(NULL == val);
            } else {
                uc_hash_node_t *node = uc_hash_find(tbl, key);
                assert(NULL == node);
            }
        }
    }

    {
        // next
        size_t ttl = uc_hash_node_ttl(tbl);
        assert(node_ttl == ttl);
        uc_hash_node_t *item = uc_hash_next(tbl, NULL);
        uc_hash_node_t *next = NULL;
        size_t times = 0;
        while (item) {
            next = uc_hash_next(tbl, uc_hash_key_addr(item));
            // uc_hash_delete(tbl, *(hash_key_t *)uc_hash_key_addr(item));
            item = next;
            times++;
            // assert(uc_hash_node_ttl(tbl) == --ttl);
        }
        assert(times == ttl);
    }

    {
        // delete
        size_t ttl = uc_hash_node_ttl(tbl);
        assert(node_ttl == ttl);
        uc_hash_node_t *item = uc_hash_next(tbl, NULL);
        uc_hash_node_t *next = NULL;
        size_t times = 0;
        while (item) {
            next = uc_hash_next(tbl, uc_hash_key_addr(item));
            uc_hash_delete(tbl, *(uc_hash_key_t *)uc_hash_key_addr(item));
            item = next;
            times++;
            // assert(uc_hash_node_ttl(tbl) == --ttl);
        }
        assert(times == ttl);
        assert(uc_hash_node_ttl(tbl) == 0);

        for (size_t i = 0; i < larget_ttl; ++i) {
            uc_hash_key_t key;
            key.i64 = i;
            uc_hash_node_t *node = uc_hash_find(tbl, key);
            assert(NULL == node);
        }
    }

    {   /* unique key and val
         * insert AND find
         */
        assert(uc_hash_node_ttl(tbl) == 0);
        uc_hash_key_t key;
        int *val = NULL;
        for (int i = 0; i < larget_ttl; ++i) {
            val = (int *)uc_hash_test_mem_malloc(sizeof(int));
            if (NULL == val) {
                printf("oom\n");
                exit(-1);
            } else {
                key.i64 = *val = i;
                uc_hash_insert(tbl, key, val);
                assert(uc_hash_node_ttl(tbl) == i+1);
            }
        }
        for (int i = 0; i < larget_ttl; ++i) {
            key.i64 = i;
            uc_hash_node_t *node = uc_hash_find(tbl, key);
            assert(node != NULL);
            assert(i == *((int *)uc_hash_node_val(node)));
        }
    }


    /* test memory */
    uc_hash_free(tbl);
    assert(0== uc_hash_test_mem_used());

    printf("0x0e6b0583 ================> table.key.int64 test done\n");
    return 0;
}

static
int test_table_key_pointer(void)
{
    printf("0x7efbb6ff <================ table.key.pointer test start\n");

    size_t larget_ttl = 1024*64;
    size_t node_ttl = 0;
    uc_hash_table_t *tbl = uc_hash_create(larget_ttl*2, uc_hash_key_pointer, 0);
    assert(tbl != NULL);

    // test insert and find
    {   int ttl = 0;
        uc_hash_key_t key;
        for (size_t i = 0; i < larget_ttl; ++i) {
            if (i % 4) {
                key.ptr = (void *)i;
                assert(key.ptr != NULL);
                ttl++;
                node_ttl++;
                int ret = uc_hash_insert(tbl, key, NULL);
                assert(ret == 0);
            }
        }
        assert(ttl == uc_hash_node_ttl(tbl));
        for (size_t i = 0; i < larget_ttl; ++i) {
            uc_hash_key_t key;
            key.ptr = (void *)i;
            if (i % 4) {
                uc_hash_node_t *node = uc_hash_find(tbl, key);
                assert(NULL != node);
                void *val = uc_hash_node_val(node);
                assert(NULL == val);
            } else {
                uc_hash_node_t *node = uc_hash_find(tbl, key);
                assert(NULL == node);
            }
        }
    }

    {
        // next
        size_t ttl = uc_hash_node_ttl(tbl);
        assert(node_ttl == ttl);
        uc_hash_node_t *item = uc_hash_next(tbl, NULL);
        uc_hash_node_t *next = NULL;
        size_t times = 0;
        while (item) {
            next = uc_hash_next(tbl, uc_hash_key_addr(item));
            item = next;
            times++;
        }
        assert(times == ttl);
    }

    {
        // delete
        size_t ttl = uc_hash_node_ttl(tbl);
        assert(node_ttl == ttl);
        uc_hash_node_t *item = uc_hash_next(tbl, NULL);
        uc_hash_node_t *next = NULL;
        size_t times = 0;
        while (item) {
            next = uc_hash_next(tbl, uc_hash_key_addr(item));
            uc_hash_delete(tbl, *(uc_hash_key_t *)uc_hash_key_addr(item));
            item = next;
            times++;
        }
        assert(times == ttl);
        assert(uc_hash_node_ttl(tbl) == 0);

        uc_hash_key_t key;
        for (size_t i = 0; i < larget_ttl; ++i) {
            key.ptr = (void *)i;
            uc_hash_node_t *node = uc_hash_find(tbl, key);
            assert(NULL == node);
        }
    }

    {   /* unique key and val
         * insert AND find
         */
        assert(uc_hash_node_ttl(tbl) == 0);
        uc_hash_key_t key;
        int *val = NULL;
        for (int64_t i = 0; i < larget_ttl; ++i) {
            val = (int *)uc_hash_test_mem_malloc(sizeof(int));
            if (NULL == val) {
                printf("oom\n");
                exit(-1);
            } else {
                *val = i;
                key.ptr = (char *)i;
                uc_hash_insert(tbl, key, val);
                assert(uc_hash_node_ttl(tbl) == i+1);
            }
        }
        for (int64_t i = 0; i < larget_ttl; ++i) {
            key.ptr = (void *)i;
            uc_hash_node_t *node = uc_hash_find(tbl, key);
            assert(node != NULL);
            assert(i == *((int *)uc_hash_node_val(node)));
        }
    }


    /* test memory */
    uc_hash_free(tbl);
    assert(0== uc_hash_test_mem_used());

    printf("0x0e6b0583 ================> table.key.ptr test done\n");
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = (test_table_void()
        || test_table_keys()
        || test_table_key_int16()
        || test_table_key_int64()
        || test_table_key_pointer());
    assert(ret == 0);
    return 0;
}
