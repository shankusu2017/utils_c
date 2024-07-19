#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hash.h"

#define ERR_OK 0

struct hash_table_s {
    hash_short_key_type_t key_type;
   	size_t key_len;

	size_t node_ttl;		/* 当前节点数 */
	size_t bucket_height;	/* 桶高 */
	hash_node_t *nodes[];
};


static size_t cal_key(void *buf, size_t len)
{
    size_t seed = 0x460613c7 + len;     /* seed */
    size_t step = (len >> 5) + 1;       /* 预防 len 过大 */
	char *data = (char *)buf;

    for (; len >= step; len -= step) {  /* compute hash */
        seed = seed ^ ((seed << 5) + (seed >> 2) + (unsigned char)(data[len - 1]));
    }

    return seed;
}

static void hash_free_node(hash_node_t *node)
{
	free(node->val);
	node->val = NULL;
	node->next = NULL;
	free(node);
    node = NULL;
}

static hash_table_t *hash_create_do(int height, hash_short_key_type_t key_type, size_t key_len)
{
	if (height <= 0 || key_len <= 0) {
		printf("0x2f670a85 len valid(%d:%d) for hash_create!", height, key_len);
		return NULL;
	}

	hash_table_t *tbl = calloc(1, sizeof(hash_table_t) + sizeof(hash_node_t*) * height);
	if (NULL == tbl) {
		printf("0x3f98e3c9 calloc mem fail for hash_create!");
		return NULL;
	}

    switch (key_type) {
    case hash_short_key_default:
        tbl->key_len = key_len;
        break;
    case hash_short_key_char:
    case hash_short_key_char_unsigned:
        tbl->key_len = sizeof(char);
        break;
    case hash_short_key_int16:
    case hash_short_key_int16_unsigned:
        tbl->key_len = sizeof(uint16_t);
        break;
    case hash_short_key_int32:
    case hash_short_key_int32_unsigned:
        tbl->key_len = sizeof(uint32_t);
        break;
    case hash_short_key_int64:
    case hash_short_key_int64_unsigned:
        tbl->key_len = sizeof(uint64_t);
        break;
    case hash_short_key_pointer:
        tbl->key_len = sizeof(void *);
        break;
    case hash_short_key_mac:
        tbl->key_len = sizeof(mac_bytes_t);
        break;
    default:
        exit(-0x65a88a7d);
        break;
    }

    tbl->key_type = key_type;
	tbl->node_ttl = 0;
	tbl->bucket_height = height;

	/* calloc 申请的内存，所以 nodes 无需再初始化 */

	return tbl;
}

hash_table_t *hash_create(int height,  size_t len)
{
    return hash_create_do(height, hash_short_key_default, len);
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
    if (tbl->key_type != hash_short_key_default) {
        exit(-0x30d2edba);
    }

	size_t h = cal_key(key, tbl->key_len) % tbl->bucket_height;
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
    if (tbl->key_type != hash_short_key_default) {
        exit(-0x7f2a4990);
    }

	size_t h = cal_key(key, tbl->key_len) % tbl->bucket_height;
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
    if (tbl->key_type != hash_short_key_default) {
        exit(-0x64f82bce);
    }

	size_t h = cal_key(key, tbl->key_len) % tbl->bucket_height;
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
	size_t h = cal_key(key, tbl->key_len) % tbl->bucket_height;
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
    if (tbl->key_type != hash_short_key_default) {
        exit(-0x6523cd0d);
    }

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



static size_t cal_short_key_len(hash_short_key_type_t type)
{
    switch (type) {
    case hash_short_key_char:
    case hash_short_key_char_unsigned:
        return sizeof(char);
        break;
    case hash_short_key_int16:
    case hash_short_key_int16_unsigned:
        return sizeof(uint16_t);
        break;
    case hash_short_key_int32:
    case hash_short_key_int32_unsigned:
        return sizeof(uint32_t);
        break;
    case hash_short_key_int64:
    case hash_short_key_int64_unsigned:
        return sizeof(uint64_t);
        break;
    case hash_short_key_pointer:
        return sizeof(void *);
        break;
    case hash_short_key_mac:
        return sizeof(mac_bytes_t);
        break;
    default:
        exit(-0x70e05102);
        break;
    }
}

static void *cal_short_key_addr(hash_short_key_t *short_key, hash_short_key_type_t type)
{
    switch (type) {
    case hash_short_key_char:
        return &short_key->c;
        break;
    case hash_short_key_char_unsigned:
        return &short_key->uc;
        break;
    case hash_short_key_int16:
        return &short_key->i16;
        break;
    case hash_short_key_int16_unsigned:
        return &short_key->u16;
        break;
    case hash_short_key_int32:
        return &short_key->i32;
        break;
    case hash_short_key_int32_unsigned:
        return &short_key->u32;
        break;
    case hash_short_key_int64:
        return &short_key->i64;
        break;
    case hash_short_key_int64_unsigned:
        return &short_key->u64;
        break;
    case hash_short_key_pointer:
        return &short_key->ptr;
        break;
    case hash_short_key_mac:
        return &short_key->mac;
        break;
    default:
        exit(-0x60bf65f1);
        break;
    }
}

static size_t cal_short_key(hash_short_key_t short_key, hash_short_key_type_t key_type)
{
    int len = cal_short_key_len(key_type);
    char *data = (char *)cal_short_key_addr(&short_key, key_type);

    return cal_key(data, len);
}

static int is_short_key_equl(hash_short_key_t keyA, hash_short_key_t keyB, hash_short_key_type_t type)
{
    switch (type) {
    case hash_short_key_char:
        return keyA.c == keyB.c;
        break;
    case hash_short_key_char_unsigned:
        return keyA.uc == keyB.uc;
        break;
    case hash_short_key_int16:
        return keyA.i16 == keyB.i16;
        break;
    case hash_short_key_int16_unsigned:
        return keyA.u16 == keyB.u16;
        break;
    case hash_short_key_int32:
        return keyA.i32 == keyB.i32;
        break;
    case hash_short_key_int32_unsigned:
        return keyA.u32 == keyB.u32;
        break;
    case hash_short_key_int64:
        return keyA.i64 == keyB.i64;
        break;
    case hash_short_key_int64_unsigned:
        return keyA.u64 == keyB.u64;
        break;
    case hash_short_key_pointer:
        return keyA.ptr == keyB.ptr;
        break;
    case hash_short_key_mac:
        return (0 == memcpy(&keyA.mac, &keyB.mac, sizeof(keyB.mac)));
        break;
    default:
        exit(-1);
        break;
    }
}

hash_table_t *hash_short_create(int height, hash_short_key_type_t short_type)
{
     return hash_create_do(height, short_type, cal_short_key_len(short_type));
}

int hash_short_insert(hash_table_t *tbl, hash_short_key_t short_key, void *val)
{
    if (tbl->key_type == hash_short_key_default) {
        exit(-0x48c068e6);
    }

	size_t h = cal_short_key(short_key, tbl->key_type) % tbl->bucket_height;
    hash_node_t *node = tbl->nodes[h];

    /* 新值替换旧值 */
    while (node) {
        if (is_short_key_equl(node->short_key, short_key, tbl->key_type)) {
			free(node->val);
            node->val = val;
            return ERR_OK;
        }
        node = node->next;
    }

	/* 插入全新值 */
	node = (hash_node_t *)malloc(sizeof(hash_node_t));
	if (NULL == node) {
		printf("0x6832762f calloc fail for hash_insert!");
		return -0x6832762f;
	}
	node->val = val;
    node->short_key = short_key;

	/* 加到链表 */
	node->next = tbl->nodes[h];
	tbl->nodes[h] = node;
	++tbl->node_ttl;

	return ERR_OK;
}

hash_node_t *hash_short_find(hash_table_t *tbl, hash_short_key_t short_key)
{
    if (tbl->key_type == hash_short_key_default) {
        exit(-0x5f6ad779);
    }

	size_t h = cal_short_key(short_key, tbl->key_type) % tbl->bucket_height;
    hash_node_t *node = tbl->nodes[h];

    while (node) {
        if (is_short_key_equl(node->short_key, short_key, tbl->key_type)) {
            return node;
        }
        node = node->next;
    }

	return NULL;
}

int hash_short_delete(hash_table_t *tbl, hash_short_key_t short_key)
{
    if (tbl->key_type == hash_short_key_default) {
        exit(-0x2f08aeda);
    }

	size_t h = cal_short_key(short_key, tbl->key_type) % tbl->bucket_height;
	hash_node_t *(*pre) = &tbl->nodes[h];
	hash_node_t *node = tbl->nodes[h];

	while (node) {
	    if (is_short_key_equl(node->short_key, short_key, tbl->key_type)) {
			*pre = node->next;
			hash_free_node(node);
			--tbl->node_ttl;
			return ERR_OK;
	    } else {
	    	pre = &node->next;
			node = node->next;
	    }
	}

	printf("0x299d823e hash_delete fail, can't find node!");
	return -0x299d823e;
}

/* 第一个 item */
hash_node_t *hash_short_head(hash_table_t *tbl)
{
    if (tbl->key_type == hash_short_key_default) {
        exit(-0x22339355);
    }

	int i = 0;
	for (i = 0; i < tbl->bucket_height; ++i) {
	    hash_node_t *node = tbl->nodes[i];
		if (node) {
			return node;
		}
	}
	printf("0x2010dc64 table is nil");
	return NULL;
}

/* 下个 item
 * 如果要完成的遍历一遍 table, 则遍历的过程中在外层需要锁住表
*/
hash_node_t *hash_short_next(hash_table_t *tbl, hash_short_key_t short_key)
{
    if (tbl->key_type == hash_short_key_default) {
        exit(-0x4885223b);
    }

	size_t h = cal_short_key(short_key, tbl->key_type) % tbl->bucket_height;
    hash_node_t *node = tbl->nodes[h];

    while (node) {
        if (is_short_key_equl(node->short_key, short_key, tbl->key_type)) {
            if (node->next) {
				return node->next;	/* 直接返回下一个 item */
            } else {
				break;	/* 本链表的最后一个 item    */
            }
        }
        node = node->next;
    }
    if (node == NULL) {	/* 传入的 key 非法, key 对应的链表找不到 key 的 item */
		printf("0x27edac9e key invalid\n");
		return NULL;
	}

	/* 从下一链开始 */
	int i = 0;
	for (i = h + 1; i < tbl->bucket_height; ++i) {
	    node = tbl->nodes[i];
		if (node) {
			return node;
		}
	}
	return NULL;
}

