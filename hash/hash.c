#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hash.h"

#define ERR_OK 0

struct hash_table_s {
    hash_key_type_t key_type;
   	size_t void_key_len;     /* 特殊键的长度 */

	size_t node_ttl;    /* 当前已存放的节点数 */
	size_t height;	    /* 桶高 */
    hash_cal_handler hash_cal_fun;  /* 计算hash的函数 */
	hash_node_t *nodes[];
};

/* 缺省的 hash 计算函数 */
static size_t hash_cal(void *buf, size_t len)
{
    size_t seed = 0x460613c7 + len;     /* seed */
    size_t step = (len >> 5) + 1;       /* 预防 len 过大 */
	char *data = (char *)buf;

    for (; len >= step; len -= step) {  /* compute hash */
        seed = seed ^ ((seed << 5) + (seed >> 2) + (unsigned char)(data[len - 1]));
    }

    return seed;
}

static size_t cal_key_len(hash_table_t *tbl)
{
    if (tbl->key_type == hash_key_void) {
        return tbl->void_key_len;
    }

    switch (tbl->key_type) {
    case hash_key_char:
    case hash_key_char_unsigned:
        return sizeof(char);
    case hash_key_int16:
    case hash_key_int16_unsigned:
        return sizeof(uint16_t);
    case hash_key_int32:
    case hash_key_int32_unsigned:
        return sizeof(uint32_t);
    case hash_key_int64:
    case hash_key_int64_unsigned:
        return sizeof(uint64_t);
    case hash_key_pointer:
        return sizeof(void *);
    case hash_key_mac:
        return sizeof(mac_bytes_t);
    default:
        printf("0x70e05102 key.type invalid %d", tbl->key_type);
        exit(-0x70e05102);
        break;
    }
}

static void *cal_key_addr(hash_table_t *tbl, struct hash_node_s *node)
{
    if (hash_key_void == tbl->key_type) {
        return node->key_void;
    }

    switch (tbl->key_type) {
    case hash_key_char:
        return &node->keys.c;
    case hash_key_char_unsigned:
        return &node->keys.uc;
    case hash_key_int16:
        return &node->keys.i16;
    case hash_key_int16_unsigned:
        return &node->keys.u16;
    case hash_key_int32:
        return &node->keys.i32;
   case hash_key_int32_unsigned:
        return &node->keys.u32;
    case hash_key_int64:
        return &node->keys.i64;
    case hash_key_int64_unsigned:
        return &node->keys.u64;
    case hash_key_pointer:
        return &node->keys.ptr;
    case hash_key_mac:
        return &node->keys.mac;
    default:
        printf("0x60bf65f1 key.type invalid %d", tbl->key_type);
        exit(-0x60bf65f1);
        break;
    }
}

static void hash_free_node(hash_node_t *node)
{
	free(node->val);
	node->val = NULL;
	node->next = NULL;
	free(node);
    node = NULL;
}

static hash_table_t *hash_create_do(size_t height, hash_key_type_t key_type, size_t void_key_len)
{
	if (height == 0) {
		printf("0x2f670a85 args valid(%d:%d:%d) for hash_create!", height, key_type, void_key_len);
		return NULL;
	}

	hash_table_t *tbl = calloc(1, sizeof(struct hash_table_s) + sizeof(struct hash_node_s*) * height);
	if (NULL == tbl) {
		printf("0x3f98e3c9 calloc mem fail for hash_create!");
		return NULL;
	}

    if (key_type == hash_key_void) {
        tbl->void_key_len = void_key_len;
    } else {
        tbl->void_key_len = 0;
    }

    tbl->key_type = key_type;
	tbl->node_ttl = 0;
	tbl->height = height;
    tbl->hash_cal_fun = hash_cal;    /* 设置缺省的 hash 计算函数 */

	/* calloc 申请的内存，所以 nodes 无需再初始化 */

	return tbl;
}

hash_table_t *hash_create(int height, size_t len)
{
    return hash_create_do(height, hash_key_void, len);
}

hash_table_t *hash_create_keys(int height, hash_key_type_t key_type)
{
     return hash_create_do(height, key_type, 0);
}

void hash_free(hash_table_t *tbl)
{
	size_t h = 0;

	for (h = 0; h < tbl->height; ++h) {
		hash_node_t *node = tbl->nodes[h];
		while (node) {
	    	hash_node_t *node_next = node->next;
			hash_free_node(node);
			node = node_next;
			--tbl->node_ttl;
		}
		tbl->nodes[h] = NULL;
	}
	/* 数组和表一起释放了，下面无需单独释放 */
	if (tbl->node_ttl != 0) {
		printf("0x0cb7eb49 free tbl error ttl: %d", tbl->node_ttl);
	}
	free(tbl);
}


int hash_insert(hash_table_t *tbl, void *key, void *val)
{
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(key, key_len) % tbl->height;
    hash_node_t *node = tbl->nodes[h];

    /* 新值替换旧值 */
    while (node) {
        if (0 == memcmp(cal_key_addr(tbl, node), key, key_len)) {
			free(node->val);
            node->val = val;
            return ERR_OK;
        }
        node = node->next;
    }

	/* 插入全新值 */
	node = (hash_node_t *)malloc(sizeof(hash_node_t) + tbl->void_key_len);
	if (NULL == node) {
		printf("0x6488e857 calloc fail for hash_insert!");
		return -0x6488e857;
	}
	node->val = val;
	memcpy(cal_key_addr(tbl, node), key, key_len);

	/* 加到链表 */
	node->next = tbl->nodes[h];
	tbl->nodes[h] = node;
	++tbl->node_ttl;

	return ERR_OK;
}

hash_node_t *hash_find(hash_table_t *tbl, void * const key)
{
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(key, key_len) % tbl->height;
    hash_node_t *node = tbl->nodes[h];

    while (node) {
        if (0 == memcmp(cal_key_addr(tbl, node), key, key_len)) {
            return node;
        }
        node = node->next;
    }

	return NULL;
}

int hash_delete(hash_table_t *tbl, void *key)
{
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(key, key_len) % tbl->height;
	hash_node_t *(*pre) = &tbl->nodes[h];
	hash_node_t *node = tbl->nodes[h];

	while (node) {
	    if (0 == memcmp(cal_key_addr(tbl, node), key, key_len)) {
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
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(key, key_len) % tbl->height;
    hash_node_t *node = tbl->nodes[h];
    while (node) {
        if (0 == memcmp(cal_key_addr(tbl, node), key, key_len)) {
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
	for (i = h+1; i < tbl->height; ++i) {
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
	for (i = 0; i < tbl->height; ++i) {
	    hash_node_t *node = tbl->nodes[i];
		if (node) {
			return node;
		}
	}
	printf("0x1f9b7a22 table is nil");
	return NULL;
}

void *hash_key_addr(hash_table_t *tbl, hash_node_t *node)
{
    return cal_key_addr(tbl, node);
}

size_t hash_node_ttl(hash_table_t *tbl)
{
	return tbl->node_ttl;
}

/* 设置 tbl 的hash计算函数 */
void hash_set_cal_hash_handler(hash_table_t *tbl, hash_cal_handler handler)
{
    tbl->hash_cal_fun = handler;
}
