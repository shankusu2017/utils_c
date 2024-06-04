#include <stdio.h>
#include "hash.h"
#include <assert.h>


#define REDIRECT_NAT_SOURCE_IP_COUNT 8
#define DIRECT_NAT_ONE_IP_PORT_MIN 1000
#define DIRECT_NAT_ONE_IP_PORT_MAX 60999
#define DIRECT_NAT_ONE_IP_PORT_COUNT ((DIRECT_NAT_ONE_IP_PORT_MAX - DIRECT_NAT_ONE_IP_PORT_MIN) + 1)
#define DIRECT_NAT_SLOT_TTL (DIRECT_NAT_ONE_IP_PORT_COUNT * REDIRECT_NAT_SOURCE_IP_COUNT)
#define SY_REDIRECT_NAT_HASH_TABLE_MAX DIRECT_NAT_SLOT_TTL

int main(int argc, char *argv[])
{
    int i, j = 0, 0;
    printf("i: %d, j: %d\n", i, j);

    printf("hello world %d\n", DIRECT_NAT_ONE_IP_PORT_COUNT);
    printf("hello world %d\n", DIRECT_NAT_SLOT_TTL);
    printf("hello world %d\n", SY_REDIRECT_NAT_HASH_TABLE_MAX);
    int key1 = 1;
    int val1 = 100;
    int val11 = 111;

    int key2 = 2;
    int val2 = 200;

    int key3 = 3;
    int val3 = 300;

    hash_table_t *tbl = hash_create(1, sizeof(int));
    hash_insert(tbl, &key1, &val1);
    hash_insert(tbl, &key1, &val11);
    assert(hash_node_count(tbl) == 1);

    hash_insert(tbl, &key2, &val2);
    assert(hash_node_count(tbl) == 2);
    hash_insert(tbl, &key3, &val3);
    assert(hash_node_count(tbl) == 3);

    hash_delete(tbl, &key1);
    assert(hash_node_count(tbl) == 2);
    hash_delete(tbl, &key3);
    assert(hash_node_count(tbl) == 1);
    hash_delete(tbl, &key3);
    assert(hash_node_count(tbl) == 1);
    hash_find(tbl, &key2);
    hash_find(tbl, &key1);
    hash_free(tbl);
}
