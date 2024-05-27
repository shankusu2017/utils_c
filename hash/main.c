#include "hash.h"
#include <assert.h>

int main(int argc, char *argv[])
{
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
