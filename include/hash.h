#ifndef HASH_H_2024_07_19_0X3D11AEFC
#define HASH_H_2024_07_19_0X3D11AEFC

/*
 * 链式 哈希表，如果 node_ttl / bucket_height 过大，则查询效率低，
 */


#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

typedef struct mac_bytes_s {
    unsigned char bytes[MAC_LEN_BYTES];
} __attribute__((packed)) mac_bytes_t;

typedef enum hash_key_type_e {
    // hash_key_void = 0,          /* 自定义长度的  void * 类型 */
    hash_key_char = 100,
    hash_key_uchar,
    hash_key_int16,
    hash_key_uint16,
    hash_key_int32,
    hash_key_uint32,
    hash_key_int64,
    hash_key_uint64,
    hash_key_pointer,   /* 指针 */
    hash_key_mac,
    hash_key_mem = 200,       /* 一片内存 */
} hash_key_type_t;

typedef union hash_key_u {
    char c;
    unsigned char uc;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    void *ptr;
    void *mem;  /* 通用类型 key */*/
    mac_bytes_t mac;
} hash_key_t;


typedef struct hash_table_s hash_table_t;
typedef struct hash_node_s hash_node_t;
typedef size_t (*hash_cal_handler)(void *addr, size_t len);

extern hash_table_t *hash_create(int bucket_height, hash_key_type_t key_type, size_t key_mem_len);
extern void hash_free(hash_table_t *tbl);

/* void 的 key 会单独复制一份，其它类型的 key 进行值拷贝
 * val 会被值拷贝，释放 tbl 时会被一起释放
 */
extern int hash_insert(hash_table_t *tbl, void *key, void *val);

/* 参考代码
 *   hash_node_t *item = hash_next(tbl, NULL);
 *   hash_node_t *next = NULL;
 *   while (item) {
 *       next = hash_next(tbl, hash_key_addr(tbl, item));
 *       hash_delete(tbl, hash_key_addr(tbl, item));
 *       item = next;
 *   }
*/
extern hash_node_t *hash_find(hash_table_t *tbl, void *key);
extern int hash_delete(hash_table_t *tbl, void *key);

/*
 * key 为空则从 head 开始找第一个item
 * 否则找到 key 后的第一个 item
 */
extern hash_node_t *hash_next(hash_table_t *tbl, void *key);

/* 更新 key 对应的值为 val
 * RETURNS: 若 key 对应的 node 不存在则直接返回 val
 * 若 key 对应的 node 存在则返回 node->val 上的旧值
 */
extern void *hash_update(hash_table_t *tbl, void *key, void *val);

extern void *hash_key_addr(hash_table_t *tbl, hash_node_t *node);

/* tbl 已存放元素个数 */
extern size_t hash_node_ttl(hash_table_t *tbl);
/* 设置独立的 tbl hash 计算函数 */
extern void hash_set_cal_hash_handler(hash_table_t *tbl, hash_cal_handler handler);

#ifdef __cplusplus
}
#endif

#endif /* HASH_H_2024_07_19_0X3D11AEFC */
