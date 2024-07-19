#ifndef HASH_H_2024_07_19_0X3D11AEFC
#define HASH_H_2024_07_19_0X3D11AEFC

/*
 * 链式 哈希表，如果 node_ttl / bucket_height 过大，则查询效率低，
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mac_bytes_s {
    unsigned char mac[6];
} __attribute__((packed)) mac_bytes_t;

typedef enum hash_key_type_e {
    hash_key_void = 0,

    hash_key_char = 100,
    hash_key_char_unsigned,
    hash_key_int16,
    hash_key_int16_unsigned,
    hash_key_int32,
    hash_key_int32_unsigned,
    hash_key_int64,
    hash_key_int64_unsigned,
    hash_key_pointer,
    hash_key_mac,
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
    struct mac_bytes_s mac;
} hash_key_t;

typedef struct hash_node_s {
	struct hash_node_s *next;

	void *val;

    /* 各种常见类型的 KEY */
    hash_key_t keys;
    /* 通用类型 void key，必须放最后 */
	char key_void[];
} hash_node_t;

typedef struct hash_table_s hash_table_t;
typedef size_t (*hash_cal_handler)(void *addr, size_t len);

extern hash_table_t *hash_create(int bucket_height, size_t key_len);
extern hash_table_t *hash_create_keys(int bucket_height, hash_key_type_t key_type);
extern void hash_free(hash_table_t *tbl);

/* void 的key会单独复制一份 key */
extern int hash_insert(hash_table_t *tbl, void *key, void *val);
extern hash_node_t *hash_find(hash_table_t *tbl, void *key);
extern int hash_delete(hash_table_t *tbl, void *key);
/*
 * key 为空则从 head 开始找第一个item
 * 否则找到 key 后的第一个 item
 */
extern hash_node_t *hash_next(hash_table_t *tbl, void *key);

extern void *hash_key_addr(hash_table_t *tbl, hash_node_t *node);

/* tbl 已存放元素个数 */
extern size_t hash_node_ttl(hash_table_t *tbl);
/* 另外设置 tbl 的 hash 计算函数 */
extern void hash_set_cal_hash_handler(hash_table_t *tbl, hash_cal_handler handler);

#ifdef __cplusplus
}
#endif

#endif /* HASH_H_2024_07_19_0X3D11AEFC */
