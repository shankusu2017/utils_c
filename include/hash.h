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

typedef union hash_short_key_u {
    char c;
    unsigned char uc;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    void *ptr;
    mac_bytes_t mac;
} hash_short_key_t;

typedef enum hash_short_key_type_e {
    hash_short_key_default = 0,

    hash_short_key_char = 100,
    hash_short_key_char_unsigned,
    hash_short_key_int16,
    hash_short_key_int16_unsigned,
    hash_short_key_int32,
    hash_short_key_int32_unsigned,
    hash_short_key_int64,
    hash_short_key_int64_unsigned,
    hash_short_key_pointer,
     hash_short_key_mac,
} hash_short_key_type_t;

typedef struct hash_node_s {
	struct hash_node_s *next;

	void *val;

    /* 长度在 sizeof(int64)内的短 key */
    hash_short_key_t short_key;

    /* 一般的长key */
	char key[];
} hash_node_t;

typedef struct hash_table_s hash_table_t;

extern hash_table_t *hash_create(int bucket_height, size_t key_len);
extern void hash_free(hash_table_t *tbl);

/* 单独复制一份 key */
extern int hash_insert(hash_table_t *tbl, void *key, void *val);
extern hash_node_t *hash_find(hash_table_t *tbl, void *key);
extern int hash_delete(hash_table_t *tbl, void *key);
extern hash_node_t *hash_next(hash_table_t *tbl, void *key);
extern size_t hash_node_ttl(hash_table_t *tbl);

/* key 长度固定且在8Byte内 */
extern hash_table_t *hash_short_create(int bucket_height, hash_short_key_type_t key_type);
extern int hash_short_insert(hash_table_t *tbl, hash_short_key_t short_key, void *val);
extern hash_node_t *hash_short_find(hash_table_t *tbl, hash_short_key_t short_key);
extern int hash_short_delete(hash_table_t *tbl, hash_short_key_t short_key);
extern hash_node_t *hash_short_head(hash_table_t *tbl);
extern hash_node_t *hash_short_next(hash_table_t *tbl, hash_short_key_t short_key);

#ifdef __cplusplus
}
#endif

#endif /* HASH_H_2024_07_19_0X3D11AEFC */
