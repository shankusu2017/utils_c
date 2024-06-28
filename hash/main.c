#include <stdio.h>
#include "hash.h"
#include <assert.h>
#include "time.h"


#define REDIRECT_NAT_SOURCE_IP_COUNT 8
#define DIRECT_NAT_ONE_IP_PORT_MIN 1000
#define DIRECT_NAT_ONE_IP_PORT_MAX 60999
#define DIRECT_NAT_ONE_IP_PORT_COUNT ((DIRECT_NAT_ONE_IP_PORT_MAX - DIRECT_NAT_ONE_IP_PORT_MIN) + 1)
#define DIRECT_NAT_SLOT_TTL (DIRECT_NAT_ONE_IP_PORT_COUNT * REDIRECT_NAT_SOURCE_IP_COUNT)
#define SY_REDIRECT_NAT_HASH_TABLE_MAX DIRECT_NAT_SLOT_TTL

int main(int argc, char *argv[])
{
    int i = 0;
    int j = 0;
    printf("i: %d, j: %d\n", i, j);

    printf("hello world %d\n", DIRECT_NAT_ONE_IP_PORT_COUNT);
    printf("hello world %d\n", DIRECT_NAT_SLOT_TTL);
    printf("hello world %d\n", SY_REDIRECT_NAT_HASH_TABLE_MAX);
    printf("sizeof(hash_node_t): %d\n", sizeof(hash_node_t));

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

    hash_table_t *tbl = hash_create(65536, sizeof(int));
    hash_insert(tbl, &key1, val1);
    hash_insert(tbl, &key1, val11);
    assert(hash_node_ttl(tbl) == 1);

    hash_insert(tbl, &key2, val2);
    assert(hash_node_ttl(tbl) == 2);
    hash_insert(tbl, &key3, val3);
    assert(hash_node_ttl(tbl) == 3);

    {
        int ttl = 0;
        void *item = hash_next(tbl, NULL);
        while (item) {
            ++ttl;
            //printf("val: %d\n", *(int *)(((hash_node_t *)item)->val));
            item = hash_next(tbl, ((hash_node_t *)item)->key);
        }
        printf("hash.item.ttl: %d\n", ttl);
    }

    printf("hash start insert\n");
    utils_printfms();
    for (int i = 0; i < 65536*4; ++i) {
        int *key_i = (int*)malloc(sizeof(int));
        if (!key_i) {
            printf("malloc fail, i: %d\n", i);
            return -1;
        }
        *key_i = i;
        int *val_i = (int *)malloc(sizeof(int));
        if (!val_i) {
            printf("malloc fail, i: %d\n", i);
            return -1;
        }
        *val_i = i;
        hash_insert(tbl, key_i, val_i);
    }
    printf("hash start next\n");
    utils_printfms();

    /* thinkpad x1 windows 环境下 耗时 30 ms */
    int ttl = 0;
    hash_node_t *item = hash_next(tbl, NULL);
    hash_node_t *next = NULL;
    while (item) {
        ++ttl;
        //printf("tbl.ttl: %d, val: %d\n", hash_node_ttl(tbl), *(int *)(item->val));
        next = hash_next(tbl, item->key);
		hash_delete(tbl, item->key);
        item = next;
    }
    printf("\nttl: %d, hash.item.ttl: %d\n", ttl, hash_node_ttl(tbl));
    utils_printfms();
    printf("\nhash next and delete done\n");


    int key_err = -0x73fed09f;
    item = hash_next(tbl, &key_err);
    assert(item == NULL);


    // hash_delete(tbl, &key1);
    // assert(hash_node_ttl(tbl) == 2);
    // hash_delete(tbl, &key3);
    // assert(hash_node_ttl(tbl) == 1);
    // hash_delete(tbl, &key3);
    // assert(hash_node_ttl(tbl) == 1);
    // hash_find(tbl, &key2);
    // hash_find(tbl, &key1);
    // hash_free(tbl);
    printf("hash test done\n");
    return 0;
}
