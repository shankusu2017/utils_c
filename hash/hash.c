#include <stdint.h>
#include <stdlib.h>
#include "hash.h"

static size_t hash_cal_key(void *buf, size_t len)
{
    size_t seed = 0xf60613c7 + len; /* seed */
    size_t step = (len >> 5) + 1;  /* 预防 len 过大 */
	char *data = (char *)buf;

    for (; len >= step; len -= step) { /* compute hash */
        seed = seed ^ ((seed << 5) + (seed >> 2) + (unsigned char)(data[len - 1]));
    }

    return seed;
}

void hash_free_node(hash_node_t *node)
{
	// free(node->key);
	//free(node->val);
	node->val = NULL;
	node->next = NULL;
	free(node);
    node = NULL;
}

hash_table_t *hash_create(int tbl_len, size_t key_len)
{
	if (tbl_len <= 0 || key_len <= 0) {
		printf("0x2f670a85 len valid(%d:%d) for hash_create !\n", tbl_len, key_len);
		return NULL;
	}
	
	hash_table_t *tbl = calloc(1, sizeof(hash_table_t) + sizeof(hash_node_t*) * tbl_len);
	if (NULL == tbl) {
		printf("0x3f98e3c9 calloc mem fail for hash_create !\n");
		return NULL;
	}

	//pthread_mutex_init(&tbl->mutex, NULL);
	tbl->key_len = key_len;
	tbl->node_count = 0;
	tbl->tbl_len = tbl_len;

	/* calloc 申请的内存，所以 nodes 无需再初始化 */

	return tbl;
}

void hash_free(hash_table_t *tbl)
{
	size_t h = 0;

	for (h = 0; h < tbl->tbl_len; ++h) {
		hash_node_t *node = tbl->nodes[h];
		while (node) {
	    	hash_node_t *node_next = node->next;
			hash_free_node(node);
			node = node_next;
			--tbl->node_count;
		}
	}
//	pthread_mutex_destroy(&tbl->mutex);
	/* 表和数组一起释放 */
	if (tbl->node_count) {
		printf("0x0cb7eb49 free tbl error ttl:%d\n", tbl->node_count);
	}
	free(tbl);
}

hash_node_t *hash_insert(hash_table_t *tbl, void *key, void *val)
{	
	size_t h = hash_cal_key(key, tbl->key_len) % tbl->tbl_len;
    hash_node_t *node = tbl->nodes[h];

    /* 新值替换旧值 */
    while (node) {
        if (0 == memcmp(node->key, key, tbl->key_len)) {
            node->val = val;
            return node;
        }
        node = node->next;
    }

	/* 插入全新值 */
	node = (hash_node_t *)calloc(1, sizeof(hash_node_t) + tbl->key_len);
	if (NULL == node) {
		printf("0x6488e857 calloc fail for hash_insert!\n");
		return NULL;
	}
	node->val = val;
	memcpy(node->key, key, tbl->key_len);
	
	/* 加到链表 */
	node->next = tbl->nodes[h];
	tbl->nodes[h] = node;
	++tbl->node_count;

	return node;
}

hash_node_t *hash_find(hash_table_t *tbl, void *key)
{
	size_t h = hash_cal_key(key, tbl->key_len) % tbl->tbl_len;
    hash_node_t *node = tbl->nodes[h];

    while (node) {
        if (0 == memcmp(node->key, key, tbl->key_len)) {
            return node;
        }
        node = node->next;
    }

	return NULL;
}

int hash_delete(hash_table_t *tbl, void *key)
{
	size_t h = hash_cal_key(key, tbl->key_len) % tbl->tbl_len;
	hash_node_t *(*pre) = &tbl->nodes[h];
	hash_node_t *node = tbl->nodes[h];

	while (node) {
	    if (0 == memcmp(node->key, key, tbl->key_len)) {
			*pre = node->next;
			hash_free_node(node);
			--tbl->node_count;
			return 0;
	    } else {
	    	pre = &node->next;
			node = node->next;
	    }
	}

	printf("0x2f62c919 hash_delete fail, can't find node!\n");
	return -1;
}

size_t hash_node_count(hash_table_t *tbl)
{
	return tbl->node_count;
}
