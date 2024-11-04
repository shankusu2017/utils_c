#include "memory.h"
#include "crc.h"
#include "hash.h"

#define ERR_OK 0

struct hash_table_s {
    hash_key_type_t key_type;
   	size_t mem_key_len;     /* 内存键的长度 */

	size_t node_ttl;    /* 当前已存放的节点数 */
	size_t height;	    /* 桶高 */
    hash_cal_handler hash_cal_fun;  /* 计算hash的函数 */
	hash_node_t *nodes[];
};


struct hash_node_s {
	struct hash_node_s *next;

	void *val;

    /* 各种常见类型的 KEY */
    hash_key_t keys;
    /********************************
     ********************************
     *** MAYBE MEMORY FOR MEM_KEY ***
     ********************************
     ********************************/
};

/* 缺省的 hash 计算函数 */
static size_t hash_cal(void *buf, size_t len)
{
    return ((size_t)util_crc64(UINT64_C(0X3C66C56408F84898), (const unsigned char *)buf, (uint64_t)len));
}

// RETURNS: 0 mean equal, others: not equal
static inline int hash_key_compare(hash_key_type_t key_type, size_t key_len, hash_key_t key_a, hash_key_t key_b)
{
    int i = 0;

    switch (key_type) {
    case hash_key_char:
        return (key_a.c == key_b.c ) ? 0 : 1;
    case hash_key_uchar:
        return (key_a.uc == key_b.uc ) ? 0 : 1;
    case hash_key_int16:
        return (key_a.i16 == key_b.i16) ? 0 : 1;
    case hash_key_uint16:
        return (key_a.u16 == key_b.u16) ? 0 : 1;
    case hash_key_int32:
        return (key_a.i32 == key_b.i32) ? 0 : 1;
    case hash_key_uint32:
        return (key_a.u32 == key_b.u32) ? 0 : 1;
    case hash_key_int64:
        return (key_a.i64 == key_b.i64) ? 0 : 1;
    case hash_key_uint64:
        return (key_a.u64 == key_b.u64) ? 0 : 1;
    case hash_key_pointer:
        return (key_a.ptr == key_b.ptr) ? 0 : 1;
    case hash_key_mac:
        for (i = 0; i < MAC_LEN_BYTES; ++i) {
            if (key_a.mac.bytes[i] != key_b.mac.bytes[i]) {
                return 1;
            }
        }
        return 0;
    case hash_key_mem:
        return memcmp(key_a.mem, key_b.mem, key_len);
    default:
        printf("0x70e05102 key.type invalid %d", tbl->key_type);
        exit(-0x70e05102);
        break;
    }
}

// RETURNS: 0 mean equal, others: not equal
static inline void hash_key_copy(hash_key_type_t key_type, size_t key_len, hash_key_t *key_a, hash_key_t key_b)
{
    int i = 0;

    switch (key_type) {
    case hash_key_char:
        key_a->c = key_b.c;
        break;
    case hash_key_uchar:
        key_a->uc = key_b.uc;
        break;
    case hash_key_int16:
        key_a->i16 = key_b.i16;
        break;
    case hash_key_uint16:
        key_a->u16 = key_b.u16;
        break;
    case hash_key_int32:
        key_a->i32 = key_b.i32;
        break;
    case hash_key_uint32:
        key_a->u32 = key_b.u32;
        break;
    case hash_key_int64:
        key_a->i64 = key_b.i64;
        break;
    case hash_key_uint64:
        key_a->u64 = key_b.u64;
        break;
    case hash_key_pointer:
        key_a->ptr = key_b.ptr;
        break;
    case hash_key_mac:
        for (i = 0; i < MAC_LEN_BYTES; ++i) {
            key_a->mac.bytes[i] = key_b.mac.bytes[i];
        }
        break;
    case hash_key_mem:
        memcpy(key_a->mem, key_b.mem, key_len);
        break;
    default:
        printf("0x70e05102 key.type invalid %d", tbl->key_type);
        exit(-0x70e05102);
        break;
    }
}

static inline size_t cal_key_len(hash_table_t *tbl)
{
    switch (tbl->key_type) {
    case hash_key_char:
    case hash_key_uchar:
        return sizeof(char);
    case hash_key_int16:
    case hash_key_uint16:
        return sizeof(uint16_t);
    case hash_key_int32:
    case hash_key_uint32:
        return sizeof(uint32_t);
    case hash_key_int64:
    case hash_key_uint64:
        return sizeof(uint64_t);
    case hash_key_pointer:
        return sizeof(void *);
    case hash_key_mac:
        return sizeof(mac_bytes_t);
    case hash_key_mem:
        return tbl->mem_key_len;
    default:
        printf("0x70e05102 key.type invalid %d", tbl->key_type);
        exit(-0x70e05102);
        break;
    }
}

static inline void *cal_key_addr2(hash_key_type_t key_type, hash_key_t key)
{
    switch (key_type) {
    case hash_key_char:
        return &key.keys.c;
    case hash_key_uchar:
        return &key.keys.uc;
    case hash_key_int16:
        return &key.keys.i16;
    case hash_key_uint16:
        return &key.keys.u16;
    case hash_key_int32:
        return &key.keys.i32;
   case hash_key_uint32:
        return &key.keys.u32;
    case hash_key_int64:
        return &key.keys.i64;
    case hash_key_uint64:
        return &key.keys.u64;
    case hash_key_pointer:
        return &key.keys.ptr;
    case hash_key_mac:
        return &key.keys.mac;
    case hash_key_mem:
        return key.keys.mem;    /* 直接返回指向的地址 */
    default:
        printf("0x11c9cb57 key.type invalid %d", tbl->key_type);
        exit(-0x11c9cb57);
        break;
    }
}


static inline void hash_free_node(hash_node_t *node)
{
	util_free(node->val);
	node->val = NULL;
	node->next = NULL;
	util_free(node);
    node = NULL;
}

static hash_table_t *hash_create_do(size_t height, hash_key_type_t key_type, size_t mem_key_len)
{
	if (height == 0) {
		printf("0x2f670a85 args valid(%ld:%d:%ld) for hash_create!", height, (int)key_type, mem_key_len);
		return NULL;
	}

	hash_table_t *tbl = util_calloc(sizeof(struct hash_table_s) + sizeof(struct hash_node_s*) * height);
	if (NULL == tbl) {
		printf("0x3f98e3c9 calloc mem fail for hash_create!");
		return NULL;
	}

    if (key_type == hash_key_mem) {
        tbl->mem_key_len = mem_key_len;
    } else {
        tbl->mem_key_len = 0;
    }

    tbl->key_type = key_type;
	tbl->node_ttl = 0;
	tbl->height = height;
    tbl->hash_cal_fun = hash_cal;    /* 设置缺省的 hash 计算函数 */

	/* calloc 申请的内存，所以 nodes 无需再初始化 */

	return tbl;
}

hash_table_t *hash_create(int height, hash_key_type_t key_type, size_t key_void_len)
{
    return hash_create_do(height, key_type, key_void_len);
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
		printf("0x0cb7eb49 free tbl error ttl: %ld", tbl->node_ttl);
	}
	util_free(tbl);
}


int hash_insert(hash_table_t *tbl, hash_key_t key, void *val)
{
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(cal_key_addr2(tbl->key_type, key), key_len) % tbl->height;
    hash_node_t *node = tbl->nodes[h];

    /* 新值替换旧值 */
    while (node) {
        if (0 == hash_key_compare(tbl->key_type, key_len, node->keys, key)) {
			util_free(node->val);
            node->val = val;
            return ERR_OK;
        }
        node = node->next;
    }

	/* 插入全新值 */
	node = (hash_node_t *)util_malloc(sizeof(hash_node_t) + tbl->mem_key_len);
	if (NULL == node) {
		printf("0x6488e857 calloc fail for hash_insert!");
		return -0x6488e857;
	}
    if (0 == tbl->mem_key_len) {
        node->keys.mem == NULL
    } else {
        node->keys.mem = node + 1;
    }
	hash_key_copy(tbl->key_type, key_len, node->keys, key);
	node->val = val;    /* 值：浅赋值 */

	/* 加到链表 */
	node->next = tbl->nodes[h];
	tbl->nodes[h] = node;
	++tbl->node_ttl;

	return ERR_OK;
}


hash_node_t *hash_find(hash_table_t *tbl, hash_key_t key)
{
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(cal_key_addr2(tbl->key_type, key), key_len) % tbl->height;
    hash_node_t *node = tbl->nodes[h];

    while (node) {
        if (0 == hash_key_compare(tbl->key_type, key_len, node->keys, key)) {
            return node;
        }
        node = node->next;
    }

	return NULL;
}

int hash_delete(hash_table_t *tbl, hash_key_t key)
{
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(cal_key_addr2(tbl->key_type, key), key_len) % tbl->height;
	hash_node_t *(*pre) = &tbl->nodes[h];
	hash_node_t *node = tbl->nodes[h];

	while (node) {
	    if (0 == hash_key_compare(tbl->key_type, key_len, node->keys, key)) {
			*pre = node->next;
			hash_free_node(node);
			--tbl->node_ttl;
			return ERR_OK;
	    } else {
	    	pre = &node->next;
			node = node->next;
	    }
	}

	printf("0x2f62c919 hash_delete fail, can't find node!\n");
	return -1;
}

void *hash_update(hash_table_t *tbl, hash_key_t key, void *val)
{
    hash_node_t *node = hash_find(tbl, key);
    if (NULL == node) {
        return NULL;
    }

    void *old = node->val;
    node->val = val;

    return old; /* 返回旧值，让调用者自己 free */
}

static hash_node_t *hash_next_node(hash_table_t *tbl, hash_key_t key)
{
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(cal_key_addr2(tbl->key_type, key), key_len) % tbl->height;
    hash_node_t *node = tbl->nodes[h];
    while (node) {
        if (0 == hash_key_compare(tbl->key_type, key_len, node->keys, key)) {
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
hash_node_t *hash_next(hash_table_t *tbl, hash_key_t *key)
{
	if (NULL != key) {
		return hash_next_node(tbl, *key);
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
    return node->keys;
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
