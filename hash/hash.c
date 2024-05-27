#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hash.h"

#define ERR_OK 0

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
	free(node->val);
	node->val = NULL;
	node->next = NULL;
	free(node);
    node = NULL;
}

hash_table_t *hash_create(int bucket_height, size_t key_len)
{
	if (bucket_height <= 0 || key_len <= 0) {
		printf("0x2f670a85 len valid(%d:%d) for hash_create!", bucket_height, key_len);
		return NULL;
	}

	hash_table_t *tbl = calloc(1, sizeof(hash_table_t) + sizeof(hash_node_t*) * bucket_height);
	if (NULL == tbl) {
		printf("0x3f98e3c9 calloc mem fail for hash_create!");
		return NULL;
	}

	//pthread_mutex_init(&tbl->mutex, NULL);
	tbl->key_len = key_len;
	tbl->node_ttl = 0;
	tbl->bucket_height = bucket_height;

	/* calloc 申请的内存，所以 nodes 无需再初始化 */

	return tbl;
}

void hash_free(hash_table_t *tbl)
{
	size_t h = 0;

	for (h = 0; h < tbl->bucket_height; ++h) {
		hash_node_t *node = tbl->nodes[h];
		while (node) {
	    	hash_node_t *node_next = node->next;
			hash_free_node(node);
			node = node_next;
			--tbl->node_ttl;
		}
		tbl->nodes[h] = NULL;
	}
	/* 数组和表一起释放 */
	if (tbl->node_ttl) {
		printf("0x0cb7eb49 free tbl error ttl: %d", tbl->node_ttl);
	}
	free(tbl);
}

int hash_insert(hash_table_t *tbl, void *key, void *val)
{
	size_t h = hash_cal_key(key, tbl->key_len) % tbl->bucket_height;
    hash_node_t *node = tbl->nodes[h];

    /* 新值替换旧值 */
    while (node) {
        if (0 == memcmp(node->key, key, tbl->key_len)) {
			free(node->val);
            node->val = val;
            return ERR_OK;
        }
        node = node->next;
    }

	/* 插入全新值 */
	node = (hash_node_t *)malloc(sizeof(hash_node_t) + tbl->key_len);
	if (NULL == node) {
		printf("0x6488e857 calloc fail for hash_insert!");
		return -0x6488e857;
	}
	node->val = val;
	memcpy(node->key, key, tbl->key_len);

	/* 加到链表 */
	node->next = tbl->nodes[h];
	tbl->nodes[h] = node;
	++tbl->node_ttl;

	return ERR_OK;
}

hash_node_t *hash_find(hash_table_t *tbl, void * const key)
{
	size_t h = hash_cal_key(key, tbl->key_len) % tbl->bucket_height;
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
	size_t h = hash_cal_key(key, tbl->key_len) % tbl->bucket_height;
	hash_node_t *(*pre) = &tbl->nodes[h];
	hash_node_t *node = tbl->nodes[h];

	while (node) {
	    if (0 == memcmp(node->key, key, tbl->key_len)) {
			*pre = node->next;
			hash_free_node(node);
			--tbl->node_ttl;
			return ERR_OK;
	    } else {
	    	pre = &node->next;
			node = node->next;
	    }
	}

	printf("0x2f62c919 hash_delete fail, can't find node!");
	return -1;
}
static hash_node_t *hash_next_node(hash_table_t *tbl, void *key)
{
	size_t h = hash_cal_key(key, tbl->key_len) % tbl->bucket_height;
    hash_node_t *node = tbl->nodes[h];
    while (node) {
        if (0 == memcmp(node->key, key, tbl->key_len)) {
            if (node->next) {
				return node->next;	/* 直接返回下一个 item */
            } else {
				break;	/* 本链表的最后一个 item    */
            }
        }
        node = node->next;
    }
	if (node == NULL) {	/* 传入的 key 非法 */
		printf("0x1b8539ae key invalid\n");
		return NULL;
	}

	/* 从下一链开始 */
	int i = 0;
	for (i = h+1; i < tbl->bucket_height; ++i) {
	    node = tbl->nodes[i];
		if (node) {
			return node;
		}
	}
	return NULL;
}

/* 下一个 item
 * key 为空则 从头开始
 * 如果要完成的遍历一遍 table, 则遍历的过程中在外层需要锁住表
*/
hash_node_t *hash_next(hash_table_t *tbl, void *key)
{
	if (NULL != key) {
		return hash_next_node(tbl, key);
	}

	int i = 0;
	for (i = 0; i < tbl->bucket_height; ++i) {
	    hash_node_t *node = tbl->nodes[i];
		if (node) {
			return node;
		}
	}
	printf("0x1f9b7a22 table is nil");
	return NULL;
}
size_t hash_node_ttl(hash_table_t *tbl)
{
	return tbl->node_ttl;
}
